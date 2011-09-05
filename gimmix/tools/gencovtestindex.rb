#!/usr/bin/ruby -w
puts <<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<title>Coverage - #{ARGV[0]}</title>
</head>
<body>

EOF

def getCoverage(file)
	total = 0
	seen = 0
	unseen = 0
	begin
		file = File.new(file + ".stats", "r")
		file.gets.scan(/^[^=]*=(\d+)$/) { |n| total = n[0].to_i }
		file.gets.scan(/^[^=]*=(\d+)$/) { |n| unseen = n[0].to_i }
		file.gets.scan(/^[^=]*=(\d+)$/) { |n| seen = n[0].to_i }
		file.close
	rescue => err
		puts "Exception: #{err}"
		err
	end
	return seen == 0 ? 0 : (100 * (seen.to_f / (unseen + seen)))
end

def printContents(base,dir = "",layer = 0)
	entries = Dir.new(base + "/" + dir).entries
	entries.sort!
	entries.each { |f|
		if !f.start_with?(".") && !(f =~ /.(html|stats)$/)
			if File.directory?(base + "/" + dir + "/" + f)
				puts "	<tr>"
				print "		<td style=\"padding-left: " + (layer * 20).to_s + "px;\">"
				puts dir + "/" + f + "</td>"
				puts "		<td>-</td>"
				puts "	</tr>"
				printContents(base,dir + "/" + f,layer + 1)
			else
				relDir = dir.length > 0 ? dir[1..-1] + "/" : ""
				puts "	<tr>"
				puts "		<td style=\"padding-left: " + (layer * 20).to_s + "px;\">"
				print "		<a href=\"src/" + relDir + f + ".html\">"
				puts relDir + File.basename(f,".gcov") + "</a>"
				puts "		</td>"
				puts "		<td>" + getCoverage(base + "/" + dir + "/" + f).to_s + "%</td>"
				puts "	</tr>"
			end
		end
	}
end

test = File.basename(File.dirname(ARGV[0]))

puts <<EOF
<h2>
	<a href="../../index.html">Coverage</a> &raquo;
	#{test}
</h2>

<table width="100%" border="1">
	<tr>
		<th>File</th>
		<th>Coverage</th>
	</tr>
EOF

printContents(ARGV[0])

puts <<EOF
</table>

</body>
</html>
EOF
