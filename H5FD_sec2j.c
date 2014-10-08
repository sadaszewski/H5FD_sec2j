#include <H5FD_sec2j.h>

#include <H5Dpublic.h>
#include <H5Ppublic.h>
#include <H5FDsec2.h>

static hid_t H5FD_SEC2j_g = 0;

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

hid_t H5FD_sec2j_init() {
    if (H5I_VFL != H5I_get_type(H5FD_SEC2j_g))
        H5FD_SEC2j_g = H5FD_register(&H5FD_sec2j_g, sizeof(H5FD_class_t), 0);

    return H5FD_SEC2j_g;
}

void H5FD_sec2j_term() {
    H5FD_SEC2j_g = 0;
}

herr_t H5Pset_fapl_sec2j(hid_t fapl_id) {
    H5P_set_driver(fapl_id, H5FD_SEC2, NULL);
}

static H5FD_t *H5FD_sec2j_open(const char *name, unsigned flags, hid_t fapl_id,
            haddr_t maxaddr) {}
static herr_t H5FD_sec2j_close(H5FD_t *_file) {}
static int H5FD_sec2j_cmp(const H5FD_t *_f1, const H5FD_t *_f2) {}
static herr_t H5FD_sec2j_query(const H5FD_t *_f1, unsigned long *flags) {}
static haddr_t H5FD_sec2j_get_eoa(const H5FD_t *_file, H5FD_mem_t type) {}
static herr_t H5FD_sec2j_set_eoa(H5FD_t *_file, H5FD_mem_t type, haddr_t addr) {}
static haddr_t H5FD_sec2j_get_eof(const H5FD_t *_file) {}
static herr_t  H5FD_sec2j_get_handle(H5FD_t *_file, hid_t fapl, void** file_handle) {}
static herr_t H5FD_sec2j_read(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
            size_t size, void *buf) {}
static herr_t H5FD_sec2j_write(H5FD_t *_file, H5FD_mem_t type, hid_t fapl_id, haddr_t addr,
            size_t size, const void *buf) {}
static herr_t H5FD_sec2j_truncate(H5FD_t *_file, hid_t dxpl_id, hbool_t closing) {}
