#!/usr/bin/python

import os 
import cgi

def printHeader( title ):
   print """Content-type: text/html

<?xml version = "1.0" encoding = "UTF-8"?>    
<html>
<head><title>%s</title></head>
<body>""" % title

printHeader( "QUERY_STRING example" )
print "<h1>Name/Value Pairs</h1>"

query = os.environ[ "QUERY_STRING" ]

if len( query ) == 0:
   print """<paragraph><br />
      Please add some name-value pairs to the URL above.
      Or try 
      <a href = "index.py?name=YourName&amp;age=23">this</a>.
      </paragraph>"""
else:   
   print """<paragraph style = "font-style: italic">
      The query string is '%s'.</paragraph>""" % cgi.escape( query )
   pairs = cgi.parse_qs( query )
   print "<p><ul>"

   for key, value in pairs.items():
      print "<li>You set '%s' to value %s" % ( key, value )
      print "</li>\n"

   print "</ul></p>"
print "</body></html>"
