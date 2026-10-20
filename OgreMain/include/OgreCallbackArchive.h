// This file is part of the OGRE project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at https://www.ogre3d.org/licensing.
// SPDX-License-Identifier: MIT
#ifndef __CallbackArchive_H__
#define __CallbackArchive_H__

#include "OgrePrerequisites.h"

#include "OgreArchive.h"
#include "OgreArchiveFactory.h"
#include "OgreHeaderPrefix.h"

namespace Ogre {

    /** \addtogroup Core
    *  @{
    */
    /** \addtogroup Resources
    *  @{
    */

#ifndef SWIG
    /** Functions the host application provides to the "Callback" archive.

        Paths are UTF-8, relative to the host's root and use '/' as separator. The callbacks may be
        called from any thread and must never throw or unwind into Ogre: they report failures through
        their return value. Results other than the handle and @c exists use 0 for success.
    */
    struct CallbackArchiveFunctions
    {
        /// Callback that receives each file found by @c list, relative to the listed directory
        typedef void (*AddEntryFunc)(void* context, const char* path, int64 size);

        /// Opens a file for reading and returns a handle, or 0 if it can not be opened
        int64 (*open)(const char* path);
        /// Reads up to @c size bytes; @c bytesRead is lower than @c size only at the end of the file
        int32 (*read)(int64 handle, void* buffer, size_t size, size_t* bytesRead);
        /// Moves the read position; @c origin is 0 (begin), 1 (current) or 2 (end)
        int32 (*seek)(int64 handle, int64 offset, int32 origin);
        int32 (*tell)(int64 handle, int64* position);
        int32 (*getLength)(int64 handle, int64* length);
        int32 (*close)(int64 handle);
        /// Returns 1 if the file exists and 0 otherwise
        int32 (*exists)(const char* path);
        /// Calls @c addEntry once for every file inside @c directory (@c "." is the root)
        int32 (*list)(const char* directory, int32 recursive, AddEntryFunc addEntry, void* context);
    };
#endif

    /** Archive whose files are provided by the host application through CallbackArchiveFunctions.

        The archive name (as passed to ResourceGroupManager::addResourceLocation) is a folder
        relative to the host's root, or "." for the root itself. The host decides where the bytes
        come from (a folder, a package, a network...) and reads them on demand, so Ogre never needs
        the files on disk.
    */
    class _OgreExport CallbackArchiveFactory : public ArchiveFactory
    {
    public:
        /// @copydoc FactoryObj::getType
        const String& getType(void) const override;

        //! @cond Doxygen_Suppress
        using ArchiveFactory::createInstance;
        //! @endcond

        Archive* createInstance(const String& name, bool readOnly) override;

        /** Sets the functions used by every "Callback" archive and registers the factory in the ArchiveManager.

            Call it after creating the Root and before adding "Callback" resource locations.
            @param functions pointer to a CallbackArchiveFunctions, which is copied
        */
        static void registerFunctions(void* functions);
    };

    /** @} */
    /** @} */

}

#include "OgreHeaderSuffix.h"
#endif
