/*
 * (C) Copyright 2011
 * Glory Hsu <glory.hsu@gmail.com>
 *
 * sshm.c - A description goes here.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <os.h>
#include <aplog.h>
#include "sshm.h"

#define SSHM_LOCK_ALIGNMENT		4
#define SSHM_LOCK_ALIGNMENT_MASK	~(SSHM_LOCK_ALIGNMENT - 1)

typedef struct {
	sem_t	sem;
} sshm_lock_t;

typedef struct {
	/* initial info */
	char	        name[128];
	int		size;		/* including lock map */
	unsigned int	flag;

	/* mapping info */
	int		fd;
	void	       *maddr;
	sshm_lock_t    *lock;
} sshm_handle;

void *sshm_open(char *name, int size, unsigned int flag)
{
	sshm_handle *sh = NULL;

	if ((name == NULL) || (size <= 0)) {
		LOGERROR("sshm(open): invalid argments.");
		return NULL;
	}

	/* init handle */
	sh = (sshm_handle *)malloc(sizeof(*sh));
	if (!sh) {
		LOGERROR("sshm(open): hd: insufficient memory.");
		return NULL;
	}
	memset(sh, 0, sizeof(*sh));

	sprintf(sh->name, "/%s", name);
	sh->size = ((sizeof(sshm_lock_t) + SSHM_LOCK_ALIGNMENT - 1) & SSHM_LOCK_ALIGNMENT_MASK)
		+ size;
	sh->flag = flag;

	/* open share memory */
	if (sh->flag & SSHM_CREAT)
		sh->fd = shm_open(sh->name, O_CREAT | O_RDWR | O_TRUNC, 0644);
	else 
		sh->fd = shm_open(sh->name, O_RDWR, 0644);
	if (sh->fd < 0) {
		LOGERROR("sshm[%s %d](open): shm_open failed.",
				sh->name, sh->size);
		free(sh);
		return NULL;
	}

	LOGINFO("sshm[%s %d](open): created at %p", sh->name, sh->size, sh);
	return sh;
}

void *sshm_init(void *hd)
{
	sshm_handle *sh = (sshm_handle *)hd;

	if (!hd)
		return NULL;

	/* fulfill space */
	if (sh->flag & SSHM_CREAT) {
		if (lseek(sh->fd, sh->size - 1, SEEK_SET) == (off_t)-1) {
			LOGERROR("sshm[%s %d](init): lseek failed.",
					sh->name, sh->size);
			goto sshm_open_fulfill_err;
		}
		if (write(sh->fd, "", 1) < 0) {
			LOGERROR("sshm[%s %d](init): write failed.",
					sh->name, sh->size);
			goto sshm_open_fulfill_err;
		}
	}

	/* do map */
	sh->maddr = mmap(NULL, sh->size, PROT_READ | PROT_WRITE,
			MAP_SHARED, sh->fd, 0);
	if (!sh->maddr) {
		LOGERROR("sshm[%s %d](init): mmap failed.",
				sh->name, sh->size);
		goto sshm_open_map_err;
	}

	/* init sem */
	sh->lock = (sshm_lock_t *)sh->maddr;
	if (sem_init(&(sh->lock->sem), 1, 1) != 0) {
		LOGERROR("sshm[%s %d](init): sem_init failed.",
				sh->name, sh->size);
		goto sshm_open_sem_init_err;
	}

	return sh->maddr + 
		((sizeof(sshm_lock_t) + SSHM_LOCK_ALIGNMENT - 1) & SSHM_LOCK_ALIGNMENT_MASK);

sshm_open_sem_init_err:
	munmap(sh->maddr, sh->size);

sshm_open_map_err:
sshm_open_fulfill_err:
	if (sh->flag & SSHM_CREAT)
		shm_unlink(sh->name);

	return NULL;
}

int sshm_lock(void *hd)
{
	sshm_handle *sh = (sshm_handle *)hd;

	if (!hd || !sh->lock)
		return -1;

	if (sem_wait(&(sh->lock->sem)) == -1) {
		LOGERROR("sshm[%s %d](lock): sem_wait failed.",
				sh->name, sh->size);
		return -1;
	}

	return 0;
}

int sshm_unlock(void *hd)
{
	sshm_handle *sh = (sshm_handle *)hd;

	if (!hd || !sh->lock)
		return -1;

	if (sem_post(&(sh->lock->sem)) == -1) {
		LOGERROR("sshm[%s %d](lock): sem_post failed.",
				sh->name, sh->size);
		return -1;
	}

	return 0;
}

void sshm_close(void *hd)
{
	sshm_handle *sh = (sshm_handle *)hd;

	if (!hd) 
		return;

	/* destroy sem */
	if ((sh->flag & SSHM_CREAT) && sh->lock) {
		sem_destroy(&(sh->lock->sem));
		sh->lock = NULL;
	}
	LOGDEBUG("sshm(close): sem destroyed.");

	/* do unmap */
	if (sh->maddr) {
		munmap(sh->maddr, sh->size);
		sh->maddr = NULL;
	}
	LOGDEBUG("sshm(close): address unmapped.");

	/* close fd */
	if (sh->fd >= 0) {
		close(sh->fd);
		sh->fd = 0;
	}
	LOGDEBUG("sshm(close): fd closed.");

	/* unlink and release */
	if (sh->flag & SSHM_CREAT) {
		shm_unlink(sh->name);
	}
	LOGDEBUG("sshm(close): %s: shm unlinked.", sh->name);
	free(sh);
	LOGDEBUG("sshm(close): handle released.");
}

