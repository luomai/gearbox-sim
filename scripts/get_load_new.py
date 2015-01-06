import sys
import csv
import os
import re
from optparse import OptionParser

if __name__ == "__main__":
  	parser = OptionParser()
	parser.add_option("-d", dest="dir", type="string", help="The entry of the results folder.")
	options, args = parser.parse_args()

	directory = options.dir
	revenue_dict = {}
	complete_ratio_dict = {}
	accept_ratio_dict = {}
	fail_ratio_dict = {}	
	for root, dirs, files in os.walk(directory):
		for file in files:
			if file.startswith('results_file'):
				with open(root + '/' + file, 'rt') as f:
                                        text = f.read()
                                        match = re.search(r'CLOUD WORKLOAD: ([\d.]+)', text)
                                        load = match.group(1)
                                        # match = re.search(r'\/([\d]+\.[\d]+)', root)
                                        # std = match.group(1)					
					match = re.search(r'TOTAL RETURN: ([\d\\+e\.]+)', text)
					revenue = match.group(1)
					revenue_dict[load] = revenue
					match = re.search(r'COMPLETE RATIO: ([\d\.]+)', text)
					ratio = match.group(1)
					complete_ratio_dict[load] = ratio
					match = re.search(r'ACCEPT RATIO: ([\d\.]+)', text)
					ratio = match.group(1)
					accept_ratio_dict[load] = ratio
					match = re.search(r'FAIL RATIO: ([\d\.]+)', text)
					ratio = match.group(1)
					fail_ratio_dict[load] = ratio

	print "\nRevenue"
	for key in sorted(revenue_dict.keys()):
		print revenue_dict[key]

	print "\nComplete ratio"
	for key in sorted(complete_ratio_dict.keys()):
		print complete_ratio_dict[key]

	print "\nAccept ratio"
	for key in sorted(accept_ratio_dict.keys()):
		print accept_ratio_dict[key]

	print "\nFail ratio"
	for key in sorted(fail_ratio_dict.keys()):
		print fail_ratio_dict[key]
