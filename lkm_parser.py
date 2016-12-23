import operator
import subprocess

def read_ksymbols(lkm_path):
	symbols = set()

	p = subprocess.Popen(['readelf', '-s', lkm_path], stdout=subprocess.PIPE)
	out = str(p.communicate())
	lines = out.split('\\n')

	for i in range(len(lines)):
		if lines[i].find('UND') > -1:
			sym = (lines[i].split(' '))[-1]
			if len(sym) > 0:
				symbols.add(sym)

	return symbols