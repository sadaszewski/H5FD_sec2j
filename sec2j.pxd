cdef extern from "H5FD_sec2j.h":
    ctypedef int hid_t

    ctypedef int herr_t

    hid_t H5FD_sec2j_init()
    void H5FD_sec2j_term()
    herr_t H5Pset_fapl_sec2j(hid_t fapl_id)

    herr_t H5FD_sec2j_tx_start(hid_t __f1)
    herr_t H5FD_sec2j_tx_end(hid_t __f1)
