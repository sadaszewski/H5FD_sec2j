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
	if os.path.exists(fname):
		fid = h5f.open(fname, fapl=fapl)
	else:
		fid = h5f.create(fname, fapl=fapl)
	print 'fid ready:', fid.id
	# f = h5py.File(fid)
	#print f



if __name__ == '__main__':
    main()