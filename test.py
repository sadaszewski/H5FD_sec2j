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
		print 'Opening the file...'
		fid = h5f.open(fname, fapl=fapl)
	else:
		fid = h5f.create(fname, flags=h5f.ACC_TRUNC, fapl=fapl)
	print 'fid ready:', fid.id
	#return
	f = h5py.File(fid)
	sec2j.tx_start(fid.id)
	g = f.require_group('bbic/volume/0')
	for i in range(10000):
		g.attrs.create('a%d' % i, 640)
	f.flush()
	os._exit(-1)
	#print f

if __name__ == '__main__':
    main()