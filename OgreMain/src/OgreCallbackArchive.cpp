// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#include "OgreStableHeaders.h"
#include "OgreCallbackArchive.h"

#include "OgreArchiveManager.h"

namespace Ogre {
namespace {
    const int32 RESULT_SUCCESS = 0;
    const int32 SEEK_ORIGIN_BEGIN = 0;
    const int32 SEEK_ORIGIN_CURRENT = 1;
    const char* ROOT_DIRECTORY = ".";

    CallbackArchiveFunctions gFunctions = {};
    bool gFunctionsRegistered = false;

    const CallbackArchiveFunctions& getFunctions()
    {
        if (!gFunctionsRegistered)
        {
            OGRE_EXCEPT(Exception::ERR_INVALID_CALL, "CallbackArchiveFactory::registerFunctions has not been called");
        }
        return gFunctions;
    }

    String normaliseDirectory(const String& name)
    {
        String directory = name;
        std::replace(directory.begin(), directory.end(), '\\', '/');
        while (!directory.empty() && directory.back() == '/')
            directory.pop_back();
        if (directory.empty())
            directory = ROOT_DIRECTORY;
        return directory;
    }

    String joinPath(const String& directory, const String& name)
    {
        if (directory == ROOT_DIRECTORY)
            return name;
        return directory + "/" + name;
    }

    struct ListedFile
    {
        String path;
        int64 size;
    };

    void addListedFile(void* context, const char* path, int64 size)
    {
        // Called from the host: an exception must not unwind through its frames
        try
        {
            static_cast<std::vector<ListedFile>*>(context)->push_back({path, size});
        }
        catch (...)
        {
        }
    }

    class CallbackDataStream : public DataStream
    {
        int64 mHandle;

    public:
        CallbackDataStream(const String& name, int64 handle, size_t size) : DataStream(name), mHandle(handle)
        {
            mSize = size;
        }

        ~CallbackDataStream() { close(); }

        size_t read(void* buf, size_t count) override
        {
            size_t bytesRead = 0;
            if (count == 0)
                return 0;
            if (mHandle == 0 || gFunctions.read(mHandle, buf, count, &bytesRead) != RESULT_SUCCESS)
            {
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot read file: " + mName);
            }
            return bytesRead;
        }

        void skip(long count) override { moveTo(count, SEEK_ORIGIN_CURRENT); }

        void seek(size_t pos) override { moveTo(int64(pos), SEEK_ORIGIN_BEGIN); }

        size_t tell(void) const override
        {
            int64 position = 0;
            if (mHandle == 0 || gFunctions.tell(mHandle, &position) != RESULT_SUCCESS)
            {
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot get the position of file: " + mName);
            }
            return size_t(position);
        }

        bool eof(void) const override { return tell() >= mSize; }

        void close(void) override
        {
            if (mHandle != 0)
            {
                gFunctions.close(mHandle);
                mHandle = 0;
            }
        }

    private:
        void moveTo(int64 offset, int32 origin)
        {
            if (mHandle == 0 || gFunctions.seek(mHandle, offset, origin) != RESULT_SUCCESS)
            {
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot seek file: " + mName);
            }
        }
    };

    class CallbackArchive : public Archive
    {
        /// Folder of the host's root that this archive exposes, without a trailing '/'
        String mDirectory;

    public:
        CallbackArchive(const String& name, const String& archType)
            : Archive(name, archType), mDirectory(normaliseDirectory(name))
        {
        }

        bool isCaseSensitive(void) const override { return false; }

        void load() override {}

        void unload() override {}

        DataStreamPtr open(const String& filename, bool readOnly = true) const override
        {
            if (!readOnly)
            {
                OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS, "Cannot open a file in read-write mode in a callback archive");
            }

            const CallbackArchiveFunctions& functions = getFunctions();
            String path = joinPath(mDirectory, filename);
            int64 handle = functions.open(path.c_str());
            if (handle == 0)
            {
                OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND, "Cannot open file: " + path);
            }

            int64 length = 0;
            if (functions.getLength(handle, &length) != RESULT_SUCCESS)
            {
                functions.close(handle);
                OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR, "Cannot get the size of file: " + path);
            }

            return std::make_shared<CallbackDataStream>(filename, handle, size_t(length));
        }

        StringVectorPtr list(bool recursive = true, bool dirs = false) const override
        {
            auto ret = std::make_shared<StringVector>();
            findFiles("*", recursive, dirs, ret.get(), 0);
            return ret;
        }

        FileInfoListPtr listFileInfo(bool recursive = true, bool dirs = false) const override
        {
            auto ret = std::make_shared<FileInfoList>();
            findFiles("*", recursive, dirs, 0, ret.get());
            return ret;
        }

        StringVectorPtr find(const String& pattern, bool recursive = true, bool dirs = false) const override
        {
            auto ret = std::make_shared<StringVector>();
            findFiles(pattern, recursive, dirs, ret.get(), 0);
            return ret;
        }

        FileInfoListPtr findFileInfo(const String& pattern, bool recursive = true, bool dirs = false) const override
        {
            auto ret = std::make_shared<FileInfoList>();
            findFiles(pattern, recursive, dirs, 0, ret.get());
            return ret;
        }

        bool exists(const String& filename) const override
        {
            if (filename.empty())
                return false;
            return getFunctions().exists(joinPath(mDirectory, filename).c_str()) == 1;
        }

        /// The host does not report modification times
        time_t getModifiedTime(const String& filename) const override { return 0; }

    private:
        /// Same rules as FileSystemArchive: the pattern may start with a folder, and the mask
        /// only applies to the base name
        void findFiles(const String& pattern, bool recursive, bool dirs, StringVector* simpleList,
                       FileInfoList* detailList) const
        {
            String normalisedPattern = pattern;
            std::replace(normalisedPattern.begin(), normalisedPattern.end(), '\\', '/');

            String patternDirectory;
            String mask = normalisedPattern;
            size_t separator = normalisedPattern.rfind('/');
            if (separator != String::npos)
            {
                patternDirectory = normalisedPattern.substr(0, separator + 1);
                mask = normalisedPattern.substr(separator + 1);
            }

            String listedDirectory = mDirectory;
            if (!patternDirectory.empty())
                listedDirectory = joinPath(mDirectory, patternDirectory.substr(0, patternDirectory.size() - 1));

            // Folders are not listed by the host, they are deduced from the file paths
            std::vector<ListedFile> files;
            bool listRecursive = recursive || dirs;
            if (getFunctions().list(listedDirectory.c_str(), listRecursive ? 1 : 0, addListedFile, &files) !=
                RESULT_SUCCESS)
                return;

            std::vector<ListedFile> entries;
            if (dirs)
            {
                std::set<String> directories;
                for (auto& file : files)
                {
                    for (size_t end = file.path.find('/'); end != String::npos; end = file.path.find('/', end + 1))
                        directories.insert(file.path.substr(0, end));
                }
                for (auto& directory : directories)
                    entries.push_back({directory, 0});
            }
            else
            {
                entries.swap(files);
            }

            for (auto& entry : entries)
            {
                String basename, path;
                StringUtil::splitFilename(entry.path, basename, path);
                if ((!recursive && !path.empty()) || !StringUtil::match(basename, mask, false))
                    continue;

                if (simpleList)
                {
                    simpleList->push_back(patternDirectory + entry.path);
                }
                else if (detailList)
                {
                    FileInfo fi;
                    fi.archive = this;
                    fi.filename = patternDirectory + entry.path;
                    fi.basename = basename;
                    fi.path = patternDirectory + path;
                    fi.compressedSize = size_t(entry.size);
                    fi.uncompressedSize = size_t(entry.size);
                    detailList->push_back(fi);
                }
            }
        }
    };
}
    //-----------------------------------------------------------------------
    const String& CallbackArchiveFactory::getType(void) const
    {
        static String name = "Callback";
        return name;
    }
    //-----------------------------------------------------------------------
    Archive* CallbackArchiveFactory::createInstance(const String& name, bool readOnly)
    {
        if (!readOnly)
            return NULL;

        return OGRE_NEW CallbackArchive(name, getType());
    }
    //-----------------------------------------------------------------------
    void CallbackArchiveFactory::registerFunctions(void* functions)
    {
        OgreAssert(functions, "functions must not be NULL");

        gFunctions = *static_cast<const CallbackArchiveFunctions*>(functions);
        gFunctionsRegistered = true;

        static CallbackArchiveFactory factory;
        ArchiveManager::getSingleton().addArchiveFactory(&factory);
    }
}
