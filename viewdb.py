"""
  Java JVMTI Trace Extraction
  by Romain Gaucher <r@rgaucher.info> - http://rgaucher.info

  Copyright (c) 2011-2012 Romain Gaucher <r@rgaucher.info>

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
"""
import sys, sqlite3
import os, re

__isiterable = lambda x: isinstance(x, basestring) or getattr(x, '__iter__', False)
__normalize_argmt = lambda x: ''.join(x.lower().split())
__normalize_paths = lambda x: [os.path.abspath(of) for of in x]
__normalize_path = lambda x: os.path.abspath(x)

WINDOW_TRACE = 20

def process(conf):
	if not os.path.isfile(conf['db']):
		raise Exception("The DB argument should point to the SQLite database")

	database = sqlite3.connect(conf['db'])
	known_fqn = {

	}
	list_threads = {}

	# List the threads
	cursor = database.cursor()
	query = "select distinct id, thread_name from threads"
	cursor.execute(query)
	for result in cursor:
		list_threads[int(result[0])] = result[1]

	bean_resolver = ('Ljavax/el/BeanELResolver;', 'getValue')
	interesting_class_query = """select fqns.id, classes.class_name, methods.method_name
	                             from fqns
	                             left join classes on classes.id = fqns.class_id
	                             left join methods on methods.id = fqns.method_id
	                             where classes.class_name = ? and methods.method_name = ?"""
	cursor = database.cursor()
	cursor.execute(interesting_class_query, bean_resolver)
	result = cursor.fetchone()

	# This is our method
	selector_fqn_id = result[0]

	# Now, let's get the traces from this guy
	trace_selector_query = """select id, thread_id, fqn_id, parent_trace_id
	                          from traces
	                          where fqn_id = ?"""
	cursor.execute(trace_selector_query, (selector_fqn_id,))
	traces_selectors = {}
	for row in cursor:
		thread_id = int(row[1])
		if thread_id not in traces_selectors:
			traces_selectors[thread_id] = []
		traces_selectors[thread_id].append(row)

	trace_info = {}
	trace_query = """select traces.id, fqns.id, classes.class_name, methods.method_name, traces.parent_trace_id
	                 from traces
	                 left join fqns on fqns.id = traces.fqn_id
	                 left join classes on classes.id = fqns.class_id
	                 left join methods on methods.id = fqns.method_id
	                 where 
	                   (? - %d - 50) < traces.id and traces.id < (? + %d) 
	                   and traces.thread_id = ? 
	                 order by traces.id asc""" % (WINDOW_TRACE, WINDOW_TRACE)
	ex = 18173553
	thid = 35
	cursor.execute(trace_query, (ex, ex, thid))
	indent = {}
	cur_indent = ""
	for row in cursor:
		parent_id = row[4]
		if parent_id not in indent:
			cur_indent += "  "
			indent[parent_id] = cur_indent
		else:
			cur_indent = indent[parent_id]
		print cur_indent, row[2], row[3]



def main(argc, argv):
	conf = {
		'db' : None
	}
	for i in range(argc):
		s = argv[i]
		if s in ('--database', '-d'):
			conf['db'] = __normalize_path(argv[i + 1])

	process(conf)


if __name__ == "__main__":
	main(len(sys.argv), sys.argv)
