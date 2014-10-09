import sys
import h5py

sys.path.append('.')

from sec2j import sec2j


def main():
    sec2j.init()
    f = h5py.File('test.h5', driver='sec2j')


if __name__ == '__main__':
    main()