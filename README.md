H5FD_sec2j
==========

H5FD_sec2j is a journalling filesystem driver for HDF5 based on SEC2 driver

The architecture od SEC2J is as follows:

A journal file format:

```
Header:

|------------------------------|-----------------------------|
| Magic string SEC2J (5 bytes) | Last entry offset (8 bytes) |
|------------------------------|-----------------------------|

Followed by entries of the following format:

|-------------------|----------------|-----------------|-
| Address (8 bytes) | Size (8 bytes) | CRC32 (4 bytes) |
|-------------------|----------------|-----------------|-

---------------------|---------------------------------|
Data (variable size) | Previous entry offset (8 bytes) |
---------------------|---------------------------------|

```

Two new functions are introduced:

```C
herr_t H5FD_sec2j_tx_start(hid_t __f1);
herr_t H5FD_sec2j_tx_end(hid_t __f1);
```

to respectively start and finish a journalled transaction on the given HDF5 file descriptor.

If the transaction gets interrupted by a process crash, upon opening the file next time the journaled changes will be reverted bringing the file to the state before transaction.

Consistency is assured by calling fsync() for both underlying SEC2 file and the journal file to ensure the journal never gets disposed before HDF5 file state is stable.

A Python wrapper using Cython is provided:

```Cython
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
```

And a simple test illustrating the problem. A simple os._exit() can ruin a regular HDF5 file without journalling!

```Python
import sys
import h5py
from h5py import h5p, h5f
import os

sys.path.append('.')

from sec2j import sec2j

def main():
	sec2j.init()
	fapl = h5p.create(h5p.FILE_ACCESS)
	print fapl
	print 'fapl.id:', fapl.id
	sec2j.set_fapl(fapl.id)
	fname = "test.h5"
	fid = h5f.create(fname, flags=h5f.ACC_TRUNC, fapl=fapl)
	print 'fid ready:', fid.id
	# return
	f = h5py.File(fid)
	sec2j.tx_start(fid.id)
	g = f.require_group('bbic/volume/0')
	g.attrs.create('width', 640)
	f.flush()
	os._exit(-1)
	#print f

if __name__ == '__main__':
    main()
```
