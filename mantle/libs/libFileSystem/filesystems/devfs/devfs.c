/*
 * File      : devfs.c
 *
 * COPYRIGHT (C) 2004-2011, RT-Thread Development Team
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
 */

#include <hdElastosMantle.h>
#include <dfs.h>
#include <dfs_fs.h>

#include "devfs.h"

struct device_dirent
{
    rt_device_t *devices;
    UInt16 read_index;
    UInt16 device_count;
};

int dfs_device_fs_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
    return DFS_STATUS_OK;
}

int dfs_device_fs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    Int32 result;
    rt_device_t dev_id;

    assert(file != NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->data;
    assert(dev_id != NULL);

    /* close device handler */
    result = rt_device_control(dev_id, cmd, args);
    if (result == RT_EOK)
        return DFS_STATUS_OK;

    return -DFS_STATUS_EIO;
}

int dfs_device_fs_read(struct dfs_fd *file, void *buf, UInt32 count)
{
    int result;
    rt_device_t dev_id;

    assert(file != NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->data;
    assert(dev_id != NULL);

    /* read device data */
    result = rt_device_read(dev_id, file->pos, buf, count);
    file->pos += result;

    return result;
}

int dfs_device_fs_write(struct dfs_fd *file, const void *buf, UInt32 count)
{
    int result;
    rt_device_t dev_id;

    assert(file != NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->data;
    assert(dev_id != NULL);

    /* read device data */
    result = rt_device_write(dev_id, file->pos, buf, count);
    file->pos += result;

    return result;
}

int dfs_device_fs_close(struct dfs_fd *file)
{
    Int32 result;
    rt_device_t dev_id;

    assert(file != NULL);

    if (file->type == FT_DIRECTORY)
    {
        struct device_dirent *root_dirent;

        root_dirent = (struct device_dirent *)file->data;
        assert(root_dirent != NULL);

        /* release dirent */
        rt_free(root_dirent);
        return DFS_STATUS_OK;
    }

    /* get device handler */
    dev_id = (rt_device_t)file->data;
    assert(dev_id != NULL);

    /* close device handler */
    result = rt_device_close(dev_id);
    if (result == RT_EOK)
    {
        file->data = NULL;

        return DFS_STATUS_OK;
    }

    return -DFS_STATUS_EIO;
}

int dfs_device_fs_open(struct dfs_fd *file)
{
    rt_device_t device;

    if (file->flags & DFS_O_CREAT)
        return -DFS_STATUS_EINVAL;

    /* open root directory */
    if ((file->path[0] == '/') && (file->path[1] == '\0') &&
        (file->flags & DFS_O_DIRECTORY))
    {
        struct rt_object *object;
        struct rt_list_node *node;
        struct rt_object_information *information;
        struct device_dirent *root_dirent;
        UInt32 count = 0;

        extern struct rt_object_information rt_object_container[];

        /* lock scheduler */
        rt_enter_critical();

        /* traverse device object */
        information = &rt_object_container[RT_Object_Class_Device];
        for (node = information->object_list.next; node != &(information->object_list); node = node->next)
        {
            count ++;
        }

        root_dirent = (struct device_dirent *)malloc(sizeof(struct device_dirent) +
            count * sizeof(rt_device_t));
        if (root_dirent != NULL)
        {
            root_dirent->devices = (rt_device_t *)(root_dirent + 1);
            root_dirent->read_index = 0;
            root_dirent->device_count = count;
            count = 0;
            /* get all device node */
            for (node = information->object_list.next; node != &(information->object_list); node = node->next)
            {
                object = rt_list_entry(node, struct rt_object, list);
                root_dirent->devices[count] = (rt_device_t)object;
                count ++;
            }
        }
        rt_exit_critical();

        /* set data */
        file->data = root_dirent;

        return DFS_STATUS_OK;
    }

    device = rt_device_find(&file->path[1]);
    if (device == NULL)
        return -DFS_STATUS_ENODEV;

    file->data = device;

    return DFS_STATUS_OK;
}

int dfs_device_fs_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    /* stat root directory */
    if ((path[0] == '/') && (path[1] == '\0'))
    {
        st->st_dev = 0;

        st->st_mode = DFS_S_IFREG | DFS_S_IRUSR | DFS_S_IRGRP | DFS_S_IROTH |
            DFS_S_IWUSR | DFS_S_IWGRP | DFS_S_IWOTH;
        st->st_mode &= ~DFS_S_IFREG;
        st->st_mode |= DFS_S_IFDIR | DFS_S_IXUSR | DFS_S_IXGRP | DFS_S_IXOTH;

        st->st_size  = 0;
        st->st_mtime = 0;
        st->st_blksize = 512;

        return DFS_STATUS_OK;
    }
    else
    {
        rt_device_t dev_id;

        dev_id = rt_device_find(&path[1]);
        if (dev_id != NULL)
        {
            st->st_dev = 0;

            st->st_mode = DFS_S_IRUSR | DFS_S_IRGRP | DFS_S_IROTH |
                DFS_S_IWUSR | DFS_S_IWGRP | DFS_S_IWOTH;

            if (dev_id->type == RT_Device_Class_Char)
                st->st_mode |= DFS_S_IFCHR;
            else if (dev_id->type == RT_Device_Class_Block)
                st->st_mode |= DFS_S_IFBLK;
            else
                st->st_mode |= DFS_S_IFREG;

            st->st_size  = 0;
            st->st_mtime = 0;
            st->st_blksize = 512;

            return DFS_STATUS_OK;
        }
    }

    return -DFS_STATUS_ENOENT;
}

int dfs_device_fs_getdents(struct dfs_fd *file, struct dirent *dirp, UInt32 count)
{
    UInt32 index;
    rt_object_t object;
    struct dirent *d;
    struct device_dirent *root_dirent;

    root_dirent = (struct device_dirent *)file->data;
    assert(root_dirent != NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -DFS_STATUS_EINVAL;

    for (index = 0; index < count && index + root_dirent->read_index < root_dirent->device_count;
        index ++)
    {
        object = (rt_object_t)root_dirent->devices[root_dirent->read_index + index];

        d = dirp + index;
        d->d_type = DFS_DT_REG;
        d->d_namlen = RT_NAME_MAX;
        d->d_reclen = (UInt16)sizeof(struct dirent);
        rt_strncpy(d->d_name, object->name, RT_NAME_MAX);
    }

    root_dirent->read_index += index;

    return index * sizeof(struct dirent);
}

static const struct dfs_filesystem_operation _device_fs =
{
    "devfs",
    DFS_FS_FLAG_DEFAULT,
    dfs_device_fs_mount,
    NULL,
    NULL,
    NULL,

    dfs_device_fs_open,
    dfs_device_fs_close,
    dfs_device_fs_ioctl,
    dfs_device_fs_read,
    dfs_device_fs_write,
    NULL,
    NULL,
    dfs_device_fs_getdents,
    NULL,
    dfs_device_fs_stat,
    NULL,
};

int devfs_init(void)
{
    /* register rom file system */
    dfs_register(&_device_fs);

    return 0;
}
INIT_FS_EXPORT(devfs_init);

