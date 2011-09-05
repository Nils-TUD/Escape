#!/usr/bin/ruby -w
puts <<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<title>Coverage - Index</title>
</head>
<body>

<h2>Coverage</h2>

<table width="100%" border="1">
	<tr>
		<th>Test</th>
		<th>Coverage</th>
	</tr>
EOF

ARGV.each { |path|
	name = File.dirname(path) + "/" + File.basename(path,".test")
	puts "	<tr>"
	puts "		<td><a href=\"" + name + "/index.html\">" + name + "</a></td>"
	puts "		<td>0%</td>"
	puts "	</tr>"
}
puts "	<tr>"
puts "		<td><a href=\"total/total/index.html\">Total</a></td>"
puts "		<td>0%</td>"
puts "	</tr>"

puts <<EOF
</table>

</body>
</html>
EOF
