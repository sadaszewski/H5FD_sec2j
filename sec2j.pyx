cimport sec2j

cdef class sec2j:
    @staticmethod
    def init():
        return H5FD_sec2j_init()

    @staticmethod
    def term():
        H5FD_sec2j_term()

    @staticmethod
    def set_fapl(hid_t fapl):
        return H5Pset_fapl_sec2j(fapl)

    @staticmethod
    def tx_start(hid_t f):
        return H5FD_sec2j_tx_start(f)

    @staticmethod
    def tx_end(hid_t f):
        return H5FD_sec2j_tx_end(f)
