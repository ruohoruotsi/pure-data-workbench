"""
Parse Pd files and emit useful information.
"""

# This program is copyright Chris McCormick, 2006 - 2009
# It started life in the gp2xPd port of Gunter Geiger's PDa
# The license on this program is LGPLv3

from operator import and_
import re

class PdParserException(Exception):
	pass

class PdParser:
	"""
	Parse Pd files using user defined filters.
	
	>>> p = PdParser("patches/parser-test.pd")
	>>> def found(canvasStack, type, action, bits):
	...   print "canvasStack:", canvasStack, "type:", type, "action:", action, "arguments:", bits
	... 
	>>> p.add_filter_method(found, canvas="REFERENCE", type="#X", action="text")
	>>> p.add_filter_method(found, canvas="semicolon-test", type="#X", action="msg")
	>>> p.add_filter_method(found, object="osc~")
	>>> p.parse()
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 135 336 Tags: tag_one \, tag_two
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 115 301 Description: Some kind of verbose description here.
	<BLANKLINE>
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 121 Name: thing
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 174 Argument 0: first argument (required)
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 194 Argument 1: second argument (optional)
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 214 Inlet 0: my first inlet is blah
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 234 Inlet 1: msg1 <msg1arg>: what this message does. msg2
	<msg2arg>: what this other message does \, default 0 which is the default
	<BLANKLINE>
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 114 278 Outlet 0: what comes out the second outlet.
	canvasStack: ['patches/parser-test.pd', 'REFERENCE'] type: #X action: text arguments: 113 141 Summary: what this thing does is blah blah blah
	canvasStack: ['patches/parser-test.pd'] type: #X action: obj arguments: 471 83 osc~ 220
	canvasStack: ['patches/parser-test.pd', 'semicolon-test'] type: #X action: msg arguments: 10 18 this is a message box \, with comments \, and also \;
	semicolons! yes it is. it's like this: yo. test \; test \; test \;
	<BLANKLINE>
	canvasStack: ['patches/parser-test.pd'] type: #X action: obj arguments: 633 213 osc~ 440
	49
	"""
	def __init__(self, filename):
		"""
		>>> p = PdParser("patches/parser-test.pd")
		"""
		# TODO: allow strings as input
		# TODO: allow file-like object as input
		# TODO: don't read the whole thing in at once, buffer up to each semicolon before parsing one line
		pfile = file(filename)
		self.contents = pfile.read()
		pfile.close()
		# the current nested canvas stack
		self.canvas = [filename]
		# list of filters to be applied to the patch
		self.filters = []
	
	def add_filter_method(self, method, **kwargs):
		"""
		The method will be called on each filter which matches for each line-element of the Pd file.
		
		The method can be passed the following variables:
		 The current canvas stack (an array), where the last element is the current canvas.
		 The type, such as "#X", "#N", etc.
		 The action, such as "canvas", "obj", "msg", "txt"
		 The first argument (such as the actual object passed) like "osc~"
		 The remaining arguments, as a string.
		
		>>> def found(canvasStack, type, action, bits):
		...   print "canvasStack:", canvasStack, "type:", type, "action:", action, "arguments:", bits
		... 
		
		For example:
		 canvas = "REFERENCE" will fire the method on each line inside that canvas.
		 type = "#X" will fire the method on each line which has a type of #X.
		 action = "obj" will fire on each line which has an 'action' of "obj.
		 object = "osc~" will fire on each line which has a first argument of 'obj~'
		
		You can chain multiple of these in the filter.
		For example, to find all comments in the subpatch called "REFERENCE":
		
		>>> p = PdParser("patches/parser-test.pd")
		>>> p.add_filter_method(found, canvas="REFERENCE", type="#X", action="text")
		"""
		self.filters.append((method, kwargs))
	
	def parse(self):
		"""
		Trigger the actual parse.
		
		>>> p = PdParser("patches/parser-test.pd")
		>>> print p.parse(), "elements found"
		49 elements found
		"""
		element_re = re.compile("(#(.*?)[^\\\]);\n", re.MULTILINE | re.DOTALL)
		# how many elements did we find?
		count = 0
		# look for the kinds of gui elements we know about
		for found in element_re.finditer(self.contents):
			count += 1
			line = found.group(1)
			bits = line.split(" ")
			# pd command type as designated by #N, #X, etc.
			type = bits.pop(0)
			# the 'action' field
			action = bits.pop(0)
			# the 'object' field
			object = len(bits) >= 3 and bits[2] or ""
			
			# check that the 'type' field is valid
			if not len(type) == 2 or not type[0] == "#":
				raise PdParserException("Type did not begin with '#' at element %d" % l)
			
			# see if our canvas stack is down a level
			if type == "#N":
				if len(bits) == 6 and action == "canvas":
					self.canvas.append(bits[4])
			
			# see if our canvas stack goes up a level
			if type == "#X":
				if len(bits) >= 3 and action == "restore":
					self.canvas.pop()
			
			# go through each of our filters, applying them to this line
			for method, filter in self.filters:
				testFilters = [("canvas", self.canvas[-1]), ("type", type), ("action", action), ("object", object)]
				if reduce(and_, [(not filter.has_key(f) or filter[f] == t) for f, t in testFilters]):
					method(self.canvas, type, action, " ".join(bits))
		return count

def _test():
	import doctest
	doctest.testmod()

if __name__ == "__main__":
	_test()

