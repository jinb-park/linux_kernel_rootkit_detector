import operator
import sys
import time
from numpy import *
from lkm_parser import *
from DataSet import DataSet
from NaiveBayes import NaiveBayes

def print_usage():
	print 'USAGE : lkrd_tool.py <command> [input file] [db file] [label]'
	print 'Example : '
	print '	lkrd_tool.py train_list rootkit.list lkrd_db.dat 1'
	print '	lkrd_tool.py train rootkit.ko lkrd_db.dat 1'
	print '	lkrd_tool.py inspect rootkit.ko lkrd_db.dat'
	exit()

def get_data(fname, dbname):
	data_str = ''
	symbols = read_ksymbols(fname)

	fr = open(dbname)
	header = fr.readlines()[0]
	header_list = header.split()
	fr.close()

	for i in range(len(header_list)):
		if header_list[i] in symbols:
			data_str += '1'
		else:
			data_str += '0'
		data_str += ' '
	return data_str

def train(fname, dbname, label):
	data_str = get_data(fname, dbname)
	dbfr = open(dbname, 'a')
	dbfr.write(data_str)
	dbfr.write(label + '\n')
	dbfr.close()

def train_list(fname, dbname, label):
	if label == '1':
		all_symbols = set()

		fr = open(fname)
		for line in fr.readlines():
			line = line.strip()
			symbols = read_ksymbols(line)
			all_symbols = all_symbols | symbols
		fr.close()

		all_symbols_list = sorted(all_symbols)
		
		dbfr = open(dbname, 'w')
		for i in range(len(all_symbols_list)):
			dbfr.write(all_symbols_list[i])
			if i == len(all_symbols_list) -1:
				#dbfr.write(' ::label')
				dbfr.write('\n')
			else:
				dbfr.write(' ')
		dbfr.close()

	fr = open(fname)
	for line in fr.readlines():
		line = line.strip()
		train(line, dbname, label)

def inspect(fname, dbname):
	classifier = NaiveBayes()
	ds_builder = DataSet(classifier)

	ds, labels, attributes = ds_builder.ReadDataSet(dbname)
	#purified_ds = ds_builder.PurifyDataSet(ds)
	trained_ds = classifier.TrainingDataSet(ds, labels, attributes)

	#
	data_str = get_data(fname, dbname)
	data_set = data_str.split()
	for i in range(len(data_set)):
		data_set[i] = float(data_set[i])

	result = classifier.InspectData(ds, trained_ds, data_set)
	return result

def check_argv(sys_argv):
	if len(sys_argv) < 2:
		print_usage()

	if sys_argv[1] == 'train_list':
		if len(sys_argv) != 5:
			print_usage()
		train_list(sys_argv[2], sys_argv[3], sys_argv[4])
	elif sys_argv[1] == 'train':
		if len(sys_argv) != 5:
			print_usage()
		train(sys_argv[2], sys_argv[3], sys_argv[4])
	elif sys_argv[1] == 'inspect':
		if len(sys_argv) != 4:
			print_usage()

		result = inspect(sys_argv[2], sys_argv[3])
		if result == 1:
			print '\"' + sys_argv[2] + '\" is rootkit'
		else:
			print '\"' + sys_argv[2] + '\" is not rootkit'
	else:
		print_usage()

if __name__ == '__main__' :
	check_argv(sys.argv)
	exit()