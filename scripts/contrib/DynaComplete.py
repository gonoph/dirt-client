#
# When ever you type a line, words from that line gets added
# to DynaComplete dictionary. When you want to use those words,
# just write few letters from start an press <tab> or
# <control space> and DynaComplete tries to complete your
# words. If nothing matches press <backspace> to erase
# that dynamic part of your text. Works only at the end of
# line.
#

import re, string

keyControlSpace = 0
keyTab = 9
keyBackspace = 127

class DynaComplete:
    def __init__( self ):
	print
	print 'DynaComplete v0.1.1 loaded. Use <conrol space> or <tab> to complete'
	print 'words you have already typed and <backspace> to erase.'
	print 'Or use "complete <words>" to add permanent completion into list.'
	self.dict = { }
	self.userwords = { }
	self.lookup = None
	self.wording = re.compile( '\s*(?P<word>[\w-#]+)' )
	self.add_word( 'complete' )

    def add_word( self, inword ):
	found = self.wording.match( inword )
	if( not found ):
	    return
	word = found.group( 'word' )
	if( len( word ) < 2 ):
	    return
	first = word[ :1 ]
	lst = [ ]
	if( self.dict.has_key( first ) ):
	    lst = self.dict[ first ]
	try:
	    found = lst.index( word )
	    del( lst[ found ] )
	except( ValueError ):
	    pass
	lst.insert( 0, word )
	self.dict[ first ] = lst

    def add_line( self, line = None ):
	global default_var
	if( not line ):
	    line = default_var
	parts = string.split( line )
	for item in parts:
	    self.add_word( item )

    def cmd_complete( self, line ):
	self.userwords[ line ] = 1
	self.add_line( line )

    def save_complete( self, fd ):
	for item in self.userwords.keys( ):
	    fd.write( "complete %s\n" % item )
	    
    def show_words( self ):
	keys = self.dict.keys( )
	for item in keys:
	    part = self.dict[ item ][ : ]
	    part.sort( )
	    print '%s: %s' % ( item, part )

    def complete( self ):
	global Key, default_var
	if( Key == keyControlSpace or Key == keyTab ):
	    Key = 0
	    parts = string.splitfields( default_var, ' ' )
	    if( self.lookup == None ):
		self.lookup = parts[ -1 ]
		recycle = 0
	    else:
		del( parts[ -1 ] )
		recycle = 1
	    length = len( self.lookup )
	    if( length > 0 ):
		first = self.lookup[ 0 ]
		try:
		    lst = self.dict[ first ]
		    if( recycle ):
			fst = lst[0]
			del( lst[ 0 ] )
			lst.append( fst )
		    self.dict[ first ] = lst
		    found = None
		    for item in lst:
			if( self.lookup == item[ : length ] ):
			    found = item
			    break
		    if( found ):
			self.add_word( found )
			found = '%s ' % found
			parts[ -1 ] = found
			default_var = string.joinfields( parts, ' ' )
		    else:
			self.lookup = None
		except( KeyError ):
		    self.lookup = None
	    else:
		self.lookup = None
	elif( Key == keyBackspace and self.lookup ):
	    Key = 0
	    parts = string.splitfields( default_var, ' ' )
	    del( parts[ -1 ] )
	    parts[ -1 ] = self.lookup
	    default_var = string.joinfields( parts, ' ' )
	    self.lookup = None
	else:
	    self.lookup = None

dynac = DynaComplete( )

def cmd_complete( param = None ):
    global default_var
    if( not param ):
	param = default_var
    dynac.cmd_complete( param )

load_add( 'complete', cmd_complete )
save_add( dynac.save_complete )
keypress_add( dynac.complete )
userinput_add( dynac.add_line )
