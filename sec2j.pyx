cimport sec2j
import h5py
from h5py import h5p, h5f
import os

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

    @staticmethod
    def open(fname):
        print 'sec2j.fopen()'
        fapl = h5p.create(h5p.FILE_ACCESS)
        sec2j.set_fapl(fapl.id)
        if os.path.exists(fname):
            fid = h5f.open(fname, fapl=fapl)
        else:
            fid = h5f.create(fname, fapl=fapl)
        print 'fid ready:', fid.id
        return h5py.File(fid)

    @staticmethod
    def set_exit(n):
        H5FD_sec2j_set_exit(n)
