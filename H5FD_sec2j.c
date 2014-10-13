//
// Journalling support for HDF5 sec2 file driver
//
// Author: Stanislaw Adaszewski, 2014
// E-mail: s.adaszewski@gmail.com
// http://algoholic.eu
//

#define _LARGEFILE64_SOURCE

#include <H5FD_sec2j.h>

#include <H5Dpublic.h>
#include <H5Ppublic.h>
#include <H5Ipublic.h>
#include <H5FDsec2.h>
#include <H5FDpublic.h>
#include <H5Epublic.h>

#include <zlib.h>

#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <malloc.h>

#define sec2j_debug printf

static hid_t H5FD_SEC2j_g = 0;
static hid_t H5FD_SEC2J_fapl = 0;
static const H5FD_class_t *sec2_cls = 0;
static int H5FD_SEC2J_exit = 0;

static H5FD_t *H5FD_sec2j_open(const char *name, unsigned flags, hid_t fapl_id,
            haddr_t maxaddr);
static herr_t H5FD_sec2j_close(H5FD_t *_file);
static int H5FD_sec2j_cmp(const H5FD_t *_f1, const H5FD_t *_f2);
static herr_t H5FD_sec2j_query(const H5FD_t *_f1, unsigned long *flags);
static haddr_t H5FD_sec2j_get_eoa(const H5FD_t *_file, H5FD_mem_t type);
static herr_t H5FD_sec2j_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr);
static haddr_t H5FD_sec2j_get_eof(const H5FD_t *_file);
static herr_t  H5FD_sec2j_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle);
static herr_t H5FD_sec2j_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
            size_t size, void *buf);
static herr_t H5FD_sec2j_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
            size_t size, const void *buf);
static herr_t H5FD_sec2j_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing);

#define MAXADDR (((haddr_t)1<<(8*sizeof(hsize_t)-1))-1)

static const H5FD_class_t H5FD_sec2j_g = {
    "sec2j",                     /* name                 */
    MAXADDR,                    /* maxaddr              */
    H5F_CLOSE_WEAK,             /* fc_degree            */
    NULL,                       /* sb_size              */
    NULL,                       /* sb_encode            */
    NULL,                       /* sb_decode            */
    0,                          /* fapl_size            */
    NULL,                       /* fapl_get             */
    NULL,                       /* fapl_copy            */
    NULL,                       /* fapl_free            */
    0,                          /* dxpl_size            */
    NULL,                       /* dxpl_copy            */
    NULL,                       /* dxpl_free            */
    H5FD_sec2j_open,             /* open                 */
    H5FD_sec2j_close,            /* close                */
    H5FD_sec2j_cmp,              /* cmp                  */
    H5FD_sec2j_query,            /* query                */
    NULL,                       /* get_type_map         */
    NULL,                       /* alloc                */
    NULL,                       /* free                 */
    H5FD_sec2j_get_eoa,          /* get_eoa              */
    H5FD_sec2j_set_eoa,          /* set_eoa              */
    H5FD_sec2j_get_eof,          /* get_eof              */
    H5FD_sec2j_get_handle,       /* get_handle           */
    H5FD_sec2j_read,             /* read                 */
    H5FD_sec2j_write,            /* write                */
    NULL,                       /* flush                */
    H5FD_sec2j_truncate,         /* truncate             */
    NULL,                       /* lock                 */
    NULL,                       /* unlock               */
    H5FD_FLMAP_DICHOTOMY        /* fl_map               */
};

typedef struct H5FD_sec2j_t {
    H5FD_t pub;
    H5FD_t *data;
    int data_fd;
    int journal_fd;
    hsize_t last_entry;
    haddr_t eoa;
    int trans;
    char journal_name[256];
} H5FD_sec2j_t;

static int H5FD_sec2j_journal_init(H5FD_sec2j_t *f1);

hid_t H5FD_sec2j_init() {
    if (H5I_VFL != H5Iget_type(H5FD_SEC2j_g)) {
        if ((H5FD_SEC2J_fapl = H5Pcreate(H5P_FILE_ACCESS)) < 0) {
            return -1;
        }

        /* if (H5Pset_fapl_sec2j(H5FD_SEC2J_fapl) < 0) {
            return -1;
        } */
        if (H5Pset_fapl_sec2(H5FD_SEC2J_fapl) < 0) {
            return -1;
        }

        sec2j_debug("H5FD_SEC2J_fapl created\n");

        H5FD_t *f = H5FDopen("sec2j_dummy.h5", H5F_ACC_CREAT | H5F_ACC_RDWR, H5P_DEFAULT, MAXADDR);

        if (!f) {
            return -1;
        }

        sec2j_debug("H5FD_SEC2J_dummy created: 0x%lX\n", (u_int64_t) f);

        sec2_cls = f->cls;

        H5FDclose(f);

        H5FD_SEC2j_g = H5FDregister(&H5FD_sec2j_g);

        sec2j_debug("SEC2J registered: %d\n", H5FD_SEC2j_g);

        /* if (H5Pset_fapl_sec2j(H5FD_SEC2J_fapl) < 0) {
            return 0;
        } */
    }

    return H5FD_SEC2j_g;
}

void H5FD_sec2j_term() {
    if (H5FD_SEC2j_g) {
        H5Pclose(H5FD_SEC2J_fapl);

        H5FDunregister(H5FD_SEC2j_g);

        H5FD_SEC2j_g = 0;
    }
}

herr_t H5Pset_fapl_sec2j(hid_t fapl_id) {
    sec2j_debug("H5Pset_fapl_sec2j()\n");
    return H5Pset_driver(fapl_id, H5FD_SEC2J, NULL);
}

herr_t H5FD_sec2j_start_transaction(H5FD_t *_f1) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;

    if (f1->trans) return -10; // transaction already in progress

    sec2j_debug("Starting transaction...\n");

    if (H5FDflush(f1->data, H5P_DATASET_XFER_DEFAULT, 0) < 0) return -15;

    f1->eoa = H5FDget_eoa(f1->data, H5FD_MEM_DEFAULT);

    if (ftruncate(f1->data_fd, H5FDget_eof(f1->data)) == -1) return -20;

    if (fsync(f1->data_fd) == -1) return -30;

    sec2j_debug("eoa: 0x%lX\n", f1->eoa);

    f1->trans = 1;

    return 0;
}

herr_t H5FD_sec2j_end_transaction(H5FD_t *_f1) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;

    if (f1->trans == 0) return -10; // no transaction in progress

    // berfore calling this HDF5 should've flushed everything

    if (H5FDflush(f1->data, H5P_DATASET_XFER_DEFAULT, 0) < 0) return -20;

    if (fsync(f1->data_fd) == -1) return -30;

    if (ftruncate(f1->journal_fd, 0) == -1) return -40;

    if (lseek64(f1->journal_fd, 0, 0) == -1) return -50;

    if (H5FD_sec2j_journal_init(f1) < 0) return -60;

    f1->trans = 0;

    return 0;
}

herr_t H5FD_sec2j_tx_start(hid_t __f1) {
    H5FD_t *_f1;

    H5Fflush(__f1, H5F_SCOPE_GLOBAL);

    if (H5Fget_vfd_handle(__f1, H5FD_SEC2J_fapl, (void**) &_f1) < 0) return -10;

    return H5FD_sec2j_start_transaction(_f1);
}

herr_t H5FD_sec2j_tx_end(hid_t __f1) {
    H5FD_t *_f1;

    if (H5Fget_vfd_handle(__f1, H5FD_SEC2J_fapl, (void**) &_f1) < 0) return -10;

    return H5FD_sec2j_end_transaction(_f1);
}

void H5FD_sec2j_set_exit(int n) {
    H5FD_SEC2J_exit = n;
}

static int H5FD_sec2j_journal_init(H5FD_sec2j_t *f1) {
    if (write(f1->journal_fd, "sec2j", 5) != 5) return -10;

    hsize_t pos = 0;

    if (write(f1->journal_fd, &pos, sizeof(hsize_t)) != sizeof(hsize_t)) return -20;

    return 0;
}

static int H5FD_sec2j_log_entry(H5FD_sec2j_t *f1, haddr_t addr, hsize_t size) {
    u_int32_t crc;
    hsize_t new_entry;
    char buf[1024];
    hsize_t n, k;
    hssize_t old_pos;

    sec2j_debug("H5FD_sec2j_log_entry(), addr: 0x%lX, size: %llu\n", addr, size);

    if (f1->trans == 0) return -1; // not in transaction mode

    if (addr >= f1->eoa) return 0; // past the sync point EOA, not interesting, good optimization

    if (f1->eoa - addr < size) size = f1->eoa - addr; // don't care after saved eao

    sec2j_debug("corrected size: %llu\n", size);

    old_pos = lseek64(f1->data_fd, 0, 1);
    if (old_pos == -1) return -5;

    if (lseek64(f1->data_fd, addr, 0) == -1) return -7;

    crc = crc32(0, 0, 0); // calculate crc
    k = size;
    while (k > 0 && (n = read(f1->data_fd, buf, k < sizeof(buf) ? k : sizeof(buf))) > 0) {
        crc = crc32(crc, (unsigned char*) buf, n);
        k -= n;
    }

    if (k != 0) {
        sec2j_debug("k: %llu\n", k);
        return -8; // not all data could be read
    }

    sec2j_debug("crc: 0x%08X\n", crc);

    new_entry = lseek64(f1->journal_fd, 0, 2);

    sec2j_debug("new_entry: %lld\n", new_entry);

    if (write(f1->journal_fd, &addr, sizeof(hsize_t)) != sizeof(hsize_t)) return -10; // write address

    if (write(f1->journal_fd, &size, sizeof(hsize_t)) != sizeof(hsize_t)) return -20; // write data size

    if (write(f1->journal_fd, &crc, sizeof(u_int32_t)) != sizeof(u_int32_t)) return -30; // write checksum

    if (lseek64(f1->data_fd, addr, 0) == -1) return -35; // couldn't seek again to correct data pos

    k = size; // write data
    while (k > 0 && (n = read(f1->data_fd, buf, k < sizeof(buf) ? k : sizeof(buf))) > 0) {
        if (write(f1->journal_fd, buf, n) != (hssize_t) n) return -40;

        hsize_t i;
        for (i = 0; i < n; i++) {
            sec2j_debug("%d ", buf[i]);
        }
        sec2j_debug("\n\n");

        k -= n;
    }

    if (k != 0) return -45; // not all data could be written

    if (write(f1->journal_fd, &f1->last_entry, sizeof(hsize_t)) != sizeof(hsize_t)) return -50; // previous entry

    if (lseek64(f1->journal_fd, 5, 0) == -1) return -55; // back to last entry offset

    if (write(f1->journal_fd, &new_entry, sizeof(hsize_t)) != sizeof(hsize_t)) return -57; // write last entry ofs

    if (fsync(f1->journal_fd) == -1) return -60; // sync journal

    f1->last_entry = new_entry;

    return 0;
}

static int H5FD_sec2j_revert_changes(int data_fd, int journal_fd) {
    char buf[1024];
    hsize_t n, k;
    u_int32_t saved_crc;
    hsize_t ofs;
    hsize_t addr_ofs;
    hsize_t size;
    u_int32_t crc;

    sec2j_debug("H5FD_sec2j_revert_changes(), data_fd: %d, journal_fd: %d\n", data_fd, journal_fd);

    if (lseek64(journal_fd, 0, 0) == -1) return -10; // set position to 0

    if (read(journal_fd, buf, 5) != 5) return -15; // read magic string
    buf[5] = 0;
    if (strcmp(buf, "sec2j") != 0) {
        sec2j_debug("Bad magic string in journal: %s\n", buf);
        return -17; // bad magic
    }

    if (read(journal_fd, &ofs, sizeof(hsize_t)) != sizeof(hsize_t)) return -20; // read position of last entry

    sec2j_debug("last_entry: %llu\n", ofs);

    while (ofs > 0) { // traverse log backwards
        if (lseek64(journal_fd, ofs, 0) == -1) return -30;

        if (read(journal_fd, &addr_ofs, sizeof(hsize_t)) != sizeof(hsize_t)) return -50; // address offset

        if (lseek64(data_fd, addr_ofs, 0) == -1) return -55; // seek data to position

        if (read(journal_fd, &size, sizeof(hsize_t)) != sizeof(hsize_t)) return -60; // data size

        if (read(journal_fd, &saved_crc, sizeof(u_int32_t)) != sizeof(u_int32_t)) return -65; // checksum

        crc = crc32(0, 0, 0); // validate checksum
        k = size;
        while (k > 0 && (n = read(journal_fd, buf, k < sizeof(buf) ? k : sizeof(buf))) > 0) {
            hsize_t i;
            for (i = 0; i < n; i++) {
                sec2j_debug("%d ", buf[i]);
            }
            sec2j_debug("\n");
            crc = crc32(crc, (unsigned char*) buf, n);
            k -= n;
        }

        sec2j_debug("Writing %lld bytes at 0x%llX, saved_crc: 0x%08X\n", size, addr_ofs, saved_crc);

        if (k != 0 || crc != saved_crc) {
            sec2j_debug("Bad crc: 0x%08X\n", crc);
            return -67; // bad crc
        }

        if (lseek64(journal_fd, ofs + sizeof(hsize_t) * 2 + sizeof(u_int32_t), 0) == -1) return -68; // seek back journal

        k = size; // write to data file
        while (k > 0 && (n = read(journal_fd, buf, k < sizeof(buf) ? k : sizeof(buf))) > 0) {
            if (write(data_fd, buf, n) != (hssize_t) n) return -70;
            k -= n;
        }

        if (read(journal_fd, &ofs, sizeof(hsize_t)) != sizeof(hsize_t)) return -75; // previous entry offset
    }

    if (fsync(data_fd) == -1) return -80; // flush to drive

    return 0;
}


static H5FD_t *H5FD_sec2j_open(const char *name, unsigned flags, hid_t fapl_id, haddr_t maxaddr)
{
    H5FD_sec2j_t *file = 0;
    int ret;
    hid_t sec2_fapl = -1;

    sec2j_debug("H5FD_sec2j_open(), name: %s, maxaddr: 0x%lX, flags: 0x%lX\n", name, maxaddr, (u_int64_t) flags);

    if (strlen(name) > 247) {
        sec2j_debug("SEC2J filename too long\n");
        goto fail;
    }

    if ((flags & H5F_ACC_RDWR) == 0) {
        sec2j_debug("Read-only access to journalled file is impossible. Journal cannot be played back.\n");
        goto fail;
    }

    if ((file = (H5FD_sec2j_t*) calloc(1, sizeof(H5FD_sec2j_t))) == 0) goto fail;

    file->journal_fd = -1;

    sprintf(file->journal_name, "%s.journal", name);

    if ((file->journal_fd = open(file->journal_name, O_RDONLY)) == -1) {
        sec2j_debug("No journal that's good\n");
    } else {
        sec2j_debug("Journal found. Reverting changes...\n");

        if ((file->data_fd = open(name, O_RDWR)) == -1) {
            sec2j_debug("Unable to open the data file\n");
            goto fail;
        }

        ret = H5FD_sec2j_revert_changes(file->data_fd, file->journal_fd);

        if (close(file->data_fd) == -1) {
            sec2j_debug("Unable to close the data file\n");
            goto fail;
        }

        if (ret < 0) {
            sec2j_debug("Failed to revert changes: %d\n", ret);
            goto fail;
        }
        sec2j_debug("Done. Removing journal...\n");
        if (close(file->journal_fd) == -1) goto fail;
        if (unlink(file->journal_name) == -1) goto fail;
    }

    if ((sec2_fapl = H5Pcopy(fapl_id)) < 0) goto fail;
    if (H5Pset_fapl_sec2(sec2_fapl) < 0) goto fail;

    if ((file->data = H5FDopen(name, flags, sec2_fapl, maxaddr)) == 0) {
        sec2j_debug("H5FDopen() failed\n");
        //H5Eprint(H5E_DEFAULT, stderr);
        //H5Eclear(H5E_DEFAULT);
        goto fail;
    }

    sec2j_debug("file->data: 0x%lX, file->data->driver_id: %d, H5FD_SEC2: %d\n", (u_int64_t) file->data, file->data->driver_id, H5FD_SEC2);

    void *fd;
    if (H5FDget_vfd_handle(file->data, H5P_DEFAULT, &fd) < 0) goto fail;

    sec2j_debug("fd: %d\n", *((int*) fd));

    file->data_fd = *((int*) fd);

    sec2j_debug("Made it here...\n");

    if ((file->journal_fd = open(file->journal_name, O_WRONLY | O_CREAT, 0600)) == -1) goto fail;

    H5FD_sec2j_journal_init(file);

    sec2j_debug("H5FD_sec2j_open() succeeded.\n");

    return (H5FD_t*) file;

fail:
    sec2j_debug("H5FD_sec2j_open() failed\n");
    if (sec2_fapl >= 0) H5Pclose(sec2_fapl);
    if (file->journal_fd != -1) close(file->journal_fd);
    if (file->data) H5FDclose(file->data);
    if (file) free(file);

    sec2j_debug("Returning 0\n");
    return 0;
}

static herr_t H5FD_sec2j_close(H5FD_t *_file) {
    H5FD_sec2j_t *file;
    herr_t ret;

    sec2j_debug("H5FD_sec2j_close()\n");

    file = ((H5FD_sec2j_t*) _file);

    if (file->trans) {
        sec2j_debug("Closing transaction\n");
        if (H5FD_sec2j_end_transaction(_file) < 0) return -10;
    }

    fsync(file->data_fd);

    if ((ret = H5FDclose(file->data)) >= 0) {
        sec2j_debug("Everything seems in order. Removing journal...\n");
        if (unlink(file->journal_name) == -1) {
            return -20;
        }
    }

    free(file);

    return ret;
}

static int H5FD_sec2j_cmp(const H5FD_t *_f1, const H5FD_t *_f2) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    H5FD_sec2j_t *f2 = (H5FD_sec2j_t*) _f2;

    return H5FDcmp(f1->data, f2->data);
}

static herr_t H5FD_sec2j_query(const H5FD_t *_f1, unsigned long *flags) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    sec2j_debug("H5FD_sec2j_query(), _f1: 0x%lX\n", (u_int64_t) _f1);
    herr_t ret = sec2_cls->query(f1 ? f1->data : 0, flags);
    sec2j_debug("Ok\n");
    return ret;
}

static haddr_t H5FD_sec2j_get_eoa(const H5FD_t *_f1, H5FD_mem_t type) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    // sec2j_debug("H5FD_sec2j_get_eoa()\n");
    return H5FDget_eoa(f1->data, type);
}

static herr_t H5FD_sec2j_set_eoa(H5FD_t *_f1, H5FD_mem_t type, haddr_t addr) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    // sec2j_debug("H5FD_sec2j_set_eoa()\n");
    return H5FDset_eoa(f1->data, type, addr);
}

static haddr_t H5FD_sec2j_get_eof(const H5FD_t *_f1) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    // sec2j_debug("H5FD_sec2j_get_eof()\n");
    return H5FDget_eof(f1->data);
}

static herr_t H5FD_sec2j_get_handle(H5FD_t *_f1, hid_t fapl, void** file_handle) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;

    if (H5P_FILE_ACCESS_DEFAULT == fapl) {
        sec2j_debug("Default get_handle\n");
        return H5FDget_vfd_handle(f1->data, fapl, file_handle);
    } else {
        sec2j_debug("Internal get_handle\n");
        *file_handle = _f1;
        return 0;
    }
}

static herr_t H5FD_sec2j_read(H5FD_t *_f1, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                size_t size, void *buf)
{
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;
    sec2j_debug("H5FD_sec2j_read()\n");
    return H5FDread(f1->data, type, fapl_id, addr, size, buf);
}

static herr_t H5FD_sec2j_write(H5FD_t *_f1, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
                size_t size, const void *buf)
{
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;

    // sec2j_debug("H5FD_sec2j_write(), type: %d\n", type);

    if (f1->trans) {
        herr_t ret;
        if ((ret = H5FD_sec2j_log_entry(f1, addr, size)) < 0) {
            sec2j_debug("H5FD_sec2j_log_entry() failed: %d\n", ret);
            return -10;
        }
    }

    if (H5FD_SEC2J_exit && rand() % 11 == 1) {
        _exit(H5FD_SEC2J_exit);
    }

    return H5FDwrite(f1->data, type, fapl_id, addr, size, buf);
}

static herr_t H5FD_sec2j_truncate(H5FD_t *_f1, hid_t dxpl_id, hbool_t closing) {
    H5FD_sec2j_t *f1 = (H5FD_sec2j_t*) _f1;

    sec2j_debug("H5FD_sec2j_truncate()\n");

    return H5FDtruncate(f1->data, dxpl_id, closing);
}
