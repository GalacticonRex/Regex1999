import re
import time

regex = open( "expressions/regex", "r" )
data = open( "text/bible", "r" )
f = open( "pyoutput", "w" )

pattern = re.compile( regex.read() )

t1 = time.time()
total = 0
for line in data:
	match = re.search(pattern, line)
	if match is not None:
		total += 1
		f.write( "%3d >>> " % total + match.group()+"\n" )
t2 = time.time()
print "Found %d Matches in %f ms" % (total,t2 - t1)