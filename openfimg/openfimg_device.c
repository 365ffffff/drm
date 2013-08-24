/*
 * Copyright (C) 2013 Tomasz Figa <tomasz.figa@gmail.com>
 *
 * Parts shamelessly copied from Freedreno driver:
 *
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "openfimg_drmif.h"
#include "openfimg_priv.h"

static pthread_mutex_t table_lock = PTHREAD_MUTEX_INITIALIZER;
static void * dev_table;

static struct of_device * of_device_new_impl(int fd)
{
	struct of_device *dev = calloc(1, sizeof(*dev));
	if (!dev)
		return NULL;
	atomic_set(&dev->refcnt, 1);
	dev->fd = fd;
	dev->handle_table = drmHashCreate();
	dev->name_table = drmHashCreate();
	return dev;
}

/* use inode for key into dev_table, rather than fd, to avoid getting
 * confused by multiple-opens:
 */
static int devkey(int fd)
{
	struct stat s;
	if (fstat(fd, &s)) {
		ERROR_MSG("stat failed: %s", strerror(errno));
		return -1;
	}
	return s.st_ino;
}

struct of_device * of_device_new(int fd)
{
	struct of_device *dev = NULL;
	int key = devkey(fd);

	pthread_mutex_lock(&table_lock);

	if (!dev_table)
		dev_table = drmHashCreate();

	if (drmHashLookup(dev_table, key, (void **)&dev)) {
		dev = of_device_new_impl(fd);
		drmHashInsert(dev_table, key, dev);
	} else {
		dev = of_device_ref(dev);
	}

	pthread_mutex_unlock(&table_lock);

	return dev;
}

struct of_device * of_device_ref(struct of_device *dev)
{
	atomic_inc(&dev->refcnt);
	return dev;
}

void of_device_del(struct of_device *dev)
{
	if (!atomic_dec_and_test(&dev->refcnt))
		return;
	pthread_mutex_lock(&table_lock);
	drmHashDestroy(dev->handle_table);
	drmHashDestroy(dev->name_table);
	drmHashDelete(dev_table, devkey(dev->fd));
	pthread_mutex_unlock(&table_lock);
	free(dev);
}
