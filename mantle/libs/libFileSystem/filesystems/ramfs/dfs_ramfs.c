/*
 * File      : dfs_ramfs.c
 *
 * COPYRIGHT (C) 2004-2013, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2013-04-15     Bernard      the first version
 * 2013-05-05     Bernard      remove CRC for ramfs persistence
 * 2013-05-22     Bernard      fix the no entry issue.
 */

#include <hdElastosMantle.h>
#include <dfs.h>
#include <dfs_fs.h>
#include "dfs_ramfs.h"

int dfs_ramfs_mount(struct dfs_filesystem *fs,
                    unsigned long          rwflag,
                    const void            *data)
{
    struct dfs_ramfs* ramfs;

    if (data == NULL)
        return -DFS_STATUS_EIO;

    ramfs = (struct dfs_ramfs *)data;
    fs->data = ramfs;

    return DFS_STATUS_OK;
}

int dfs_ramfs_unmount(struct dfs_filesystem *fs)
{
    fs->data = NULL;

    return DFS_STATUS_OK;
}

int dfs_ramfs_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    struct dfs_ramfs *ramfs;

    ramfs = (struct dfs_ramfs *)fs->data;
    assert(ramfs != NULL);
    assert(buf != NULL);

    buf->f_bsize  = 512;
    buf->f_blocks = ramfs->memheap.pool_size/512;
    buf->f_bfree  = ramfs->memheap.available_size/512;

    return DFS_STATUS_OK;
}

int dfs_ramfs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    return -DFS_STATUS_EIO;
}

struct ramfs_dirent *dfs_ramfs_lookup(struct dfs_ramfs *ramfs,
                                      const char       *path,
                                      UInt32        *size)
{
    const char *subpath;
    struct ramfs_dirent *dirent;

    subpath = path;
    while (*subpath == '/' && *subpath)
        subpath ++;
    if (! *subpath) /* is root directory */
    {
        *size = 0;

        return &(ramfs->root);
    }

    for (dirent = rt_list_entry(ramfs->root.list.next, struct ramfs_dirent, list);
         dirent != &(ramfs->root);
         dirent = rt_list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        if (rt_strcmp(dirent->name, subpath) == 0)
        {
            *size = dirent->size;

            return dirent;
        }
    }

    /* not found */
    return NULL;
}

int dfs_ramfs_read(struct dfs_fd *file, void *buf, UInt32 count)
{
    UInt32 length;
    struct ramfs_dirent *dirent;

    dirent = (struct ramfs_dirent *)file->data;
    assert(dirent != NULL);

    if (count < file->size - file->pos)
        length = count;
    else
        length = file->size - file->pos;

    if (length > 0)
        memcpy(buf, &(dirent->data[file->pos]), length);

    /* update file current position */
    file->pos += length;

    return length;
}

int dfs_ramfs_write(struct dfs_fd *fd, const void *buf, UInt32 count)
{
    struct ramfs_dirent *dirent;
    struct dfs_ramfs *ramfs;

    ramfs = (struct dfs_ramfs*)fd->fs->data;
    assert(ramfs != NULL);
    dirent = (struct ramfs_dirent*)fd->data;
    assert(dirent != NULL);

    if (count + fd->pos > fd->size)
    {
        UInt8 *ptr;
        ptr = rt_memheap_realloc(&(ramfs->memheap), dirent->data, fd->pos + count);
        if (ptr == NULL)
        {
            rt_set_errno(-RT_ENOMEM);

            return 0;
        }

        /* update dirent and file size */
        dirent->data = ptr;
        dirent->size = fd->pos + count;
        fd->size = dirent->size;
    }

    if (count > 0)
        memcpy(dirent->data + fd->pos, buf, count);

    /* update file current position */
    fd->pos += count;

    return count;
}

int dfs_ramfs_lseek(struct dfs_fd *file, Int32 offset)
{
    if (offset <= (Int32)file->size)
    {
        file->pos = offset;

        return file->pos;
    }

    return -DFS_STATUS_EIO;
}

int dfs_ramfs_close(struct dfs_fd *file)
{
    file->data = NULL;

    return DFS_STATUS_OK;
}

int dfs_ramfs_open(struct dfs_fd *file)
{
    UInt32 size;
    struct dfs_ramfs *ramfs;
    struct ramfs_dirent *dirent;

    ramfs = (struct dfs_ramfs *)file->fs->data;
    assert(ramfs != NULL);

    if (file->flags & DFS_O_DIRECTORY)
    {
        if (file->flags & DFS_O_CREAT)
        {
            return -DFS_STATUS_ENOSPC;
        }

        /* open directory */
        dirent = dfs_ramfs_lookup(ramfs, file->path, &size);
        if (dirent == NULL)
            return -DFS_STATUS_ENOENT;
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            if (!(file->flags & DFS_O_DIRECTORY))
            {
                return -DFS_STATUS_ENOENT;
            }
        }
    }
    else
    {
        dirent = dfs_ramfs_lookup(ramfs, file->path, &size);
        if (dirent == &(ramfs->root)) /* it's root directory */
        {
            return -DFS_STATUS_ENOENT;
        }

        if (dirent == NULL)
        {
            if (file->flags & DFS_O_CREAT || file->flags & DFS_O_WRONLY)
            {
                char *name_ptr;

                /* create a file entry */
                dirent = (struct ramfs_dirent *)
                         rt_memheap_alloc(&(ramfs->memheap),
                                          sizeof(struct ramfs_dirent));
                if (dirent == NULL)
                {
                    return -DFS_STATUS_ENOMEM;
                }

                /* remove '/' separator */
                name_ptr = file->path;
                while (*name_ptr == '/' && *name_ptr)
                    name_ptr ++;
                strncpy(dirent->name, name_ptr, RAMFS_NAME_MAX);

                rt_list_init(&(dirent->list));
                dirent->data = NULL;
                dirent->size = 0;
                /* add to the root directory */
                rt_list_insert_after(&(ramfs->root.list), &(dirent->list));
            }
            else
                return -DFS_STATUS_ENOENT;
        }

        /* Creates a new file.
         * If the file is existing, it is truncated and overwritten.
         */
        if (file->flags & DFS_O_TRUNC)
        {
            dirent->size = 0;
            if (dirent->data != NULL)
            {
                rt_memheap_free(dirent->data);
                dirent->data = NULL;
            }
        }
    }

    file->data = dirent;
    file->size = dirent->size;
	if (file->flags & DFS_O_APPEND)
		file->pos = file->size;
	else
		file->pos = 0;

    return DFS_STATUS_OK;
}

int dfs_ramfs_stat(struct dfs_filesystem *fs,
                   const char            *path,
                   struct stat           *st)
{
    UInt32 size;
    struct ramfs_dirent *dirent;
    struct dfs_ramfs *ramfs;

    ramfs = (struct dfs_ramfs *)fs->data;
    dirent = dfs_ramfs_lookup(ramfs, path, &size);

    if (dirent == NULL)
        return -DFS_STATUS_ENOENT;

    st->st_dev = 0;
    st->st_mode = DFS_S_IFREG | DFS_S_IRUSR | DFS_S_IRGRP | DFS_S_IROTH |
                  DFS_S_IWUSR | DFS_S_IWGRP | DFS_S_IWOTH;

    st->st_size = dirent->size;
    st->st_mtime = 0;
    st->st_blksize = 512;

    return DFS_STATUS_OK;
}

int dfs_ramfs_getdents(struct dfs_fd *file,
                       struct dirent *dirp,
                       UInt32    count)
{
    UInt32 index, end;
    struct dirent *d;
    struct ramfs_dirent *dirent;
    struct dfs_ramfs *ramfs;

    ramfs  = (struct dfs_ramfs *)file->fs->data;
    dirent = (struct ramfs_dirent *)file->data;
    if (dirent != &(ramfs->root))
        return -DFS_STATUS_EINVAL;

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -DFS_STATUS_EINVAL;

    end = file->pos + count;
    index = 0;
    count = 0;
    for (dirent = rt_list_entry(dirent->list.next, struct ramfs_dirent, list);
         dirent != &(ramfs->root) && index < end;
         dirent = rt_list_entry(dirent->list.next, struct ramfs_dirent, list))
    {
        if (index >= (UInt32)file->pos)
        {
            d = dirp + count;
            d->d_type = DFS_DT_REG;
            d->d_namlen = RT_NAME_MAX;
            d->d_reclen = (UInt16)sizeof(struct dirent);
            rt_strncpy(d->d_name, dirent->name, RAMFS_NAME_MAX);

            count += 1;
            file->pos += 1;
        }
        index += 1;
    }

    return count * sizeof(struct dirent);
}

int dfs_ramfs_unlink(struct dfs_filesystem *fs, const char *path)
{
    UInt32 size;
    struct dfs_ramfs *ramfs;
    struct ramfs_dirent *dirent;

    ramfs = (struct dfs_ramfs *)fs->data;
    assert(ramfs != NULL);

    dirent = dfs_ramfs_lookup(ramfs, path, &size);
    if (dirent == NULL)
        return -DFS_STATUS_ENOENT;

    rt_list_remove(&(dirent->list));
    if (dirent->data != NULL)
        rt_memheap_free(dirent->data);
    rt_memheap_free(dirent);

    return DFS_STATUS_OK;
}

int dfs_ramfs_rename(struct dfs_filesystem *fs,
                     const char            *oldpath,
                     const char            *newpath)
{
    struct ramfs_dirent *dirent;
    struct dfs_ramfs *ramfs;
    UInt32 size;

    ramfs = (struct dfs_ramfs *)fs->data;
    assert(ramfs != NULL);

    dirent = dfs_ramfs_lookup(ramfs, newpath, &size);
    if (dirent != NULL)
        return -DFS_STATUS_EEXIST;

    dirent = dfs_ramfs_lookup(ramfs, oldpath, &size);
    if (dirent == NULL)
        return -DFS_STATUS_ENOENT;

    strncpy(dirent->name, newpath, RAMFS_NAME_MAX);

    return DFS_STATUS_OK;
}

static const struct dfs_filesystem_operation _ramfs =
{
    "ram",
    DFS_FS_FLAG_DEFAULT,
    dfs_ramfs_mount,
    dfs_ramfs_unmount,
    NULL, /* mkfs */
    dfs_ramfs_statfs,

    dfs_ramfs_open,
    dfs_ramfs_close,
    dfs_ramfs_ioctl,
    dfs_ramfs_read,
    dfs_ramfs_write,
    NULL, /* flush */
    dfs_ramfs_lseek,
    dfs_ramfs_getdents,
    dfs_ramfs_unlink,
    dfs_ramfs_stat,
    dfs_ramfs_rename,
};

int dfs_ramfs_init(void)
{
    /* register ram file system */
    dfs_register(&_ramfs);

    return 0;
}
INIT_FS_EXPORT(dfs_ramfs_init);

struct dfs_ramfs* dfs_ramfs_create(UInt8 *pool, UInt32 size)
{
    struct dfs_ramfs *ramfs;
    UInt8 *data_ptr;
    Int32 result;

    size  = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);
    ramfs = (struct dfs_ramfs *)pool;

    data_ptr = (UInt8 *)(ramfs + 1);
    size = size - sizeof(struct dfs_ramfs);
    size = RT_ALIGN_DOWN(size, RT_ALIGN_SIZE);

    result = rt_memheap_init(&ramfs->memheap, "ramfs", data_ptr, size);
    if (result != RT_EOK)
        return NULL;
    /* detach this memheap object from the system */
    rt_object_detach((rt_object_t)&(ramfs->memheap));

    /* initialize ramfs object */
    ramfs->magic = RAMFS_MAGIC;

    /* initialize root directory */
    memset(&(ramfs->root), 0x00, sizeof(ramfs->root));
    rt_list_init(&(ramfs->root.list));
    ramfs->root.size = 0;
    strcpy(ramfs->root.name, ".");

    return ramfs;
}
