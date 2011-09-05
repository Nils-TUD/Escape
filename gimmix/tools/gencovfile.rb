#!/usr/bin/ruby -w
base = File.dirname(File.dirname(ARGV[1]))
parent = ARGV[1]
parentName = File.basename(ARGV[1])
file = ARGV[0]
absFile = ARGV[1] + "/" + ARGV[0]

puts <<EOF
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
	<title>Coverage - #{file}</title>
	<style type="text/css">
	body {
		margin: 10px;
	}
	.code {
		font-family: monospace, 'Courier New', Courier;
		font-size: 13px;
		line-height: 18px;
		font-weight: bold;
		border: 1px solid #555;
		padding: 5px;
	}
	.seen {
		background-color: green;
		color: white;
	}
	.notseen {
		background-color: red;
		color: white;
	}
	.default {
	}
	</style>
</head>
<body>

<h2>
	<a href="#{base}/index.html">Coverage</a> &raquo;
	<a href="#{parent}/index.html">#{parentName}</a> &raquo;
	#{file}:
</h2>
EOF

total = 0
seen = 0
unseen = 0
begin
	file = File.new(absFile + ".stats", "r")
	file.gets.scan(/^[^=]*=(\d+)$/) { |n| total = n[0].to_i }
	file.gets.scan(/^[^=]*=(\d+)$/) { |n| unseen = n[0].to_i }
	file.gets.scan(/^[^=]*=(\d+)$/) { |n| seen = n[0].to_i }
	file.close
rescue => err
	puts "Exception: #{err}"
	err
end

executable = seen + unseen
coverage = seen == 0 ? 0 : (100 * (seen.to_f / (unseen + seen)))

puts <<EOF

<table width="100%" border="1">
	<tr>
		<td>Total lines:</td>
		<td>#{total}</td>
	</tr>
	<tr>
		<td>Executable lines:</td>
		<td>#{executable}</td>
	</tr>
	<tr>
		<td>Executed lines:</td>
		<td>#{seen}</td>
	</tr>
	<tr>
		<td>Coverage:</td>
		<td>#{coverage}%</td>
	</tr>
</table>
<br />

<div class="code">
EOF

begin
	file = File.new(absFile, "r")
	while (line = file.gets)
		line.scan(/^\W*(\d+|\-|\#+):\W*(\d+):(.*)$/) { |cov,no,line|
			if no.to_i > 0
				line.gsub!(/&/,"&amp;")
				line.gsub!(/\t/,"&nbsp;" * 3)
				line.gsub!(/ /,"&nbsp;")
				line.gsub!(/</,"&lt;")
				line.gsub!(/>/,"&gt;")
				print "<span class=\""
				print cov == "#" ? "notseen" : cov == "-" ? "default" : "seen"
				puts "\">" + line + "</span><br />"
			end
		}
	end
	file.close
rescue => err
	puts "Exception: #{err}"
	err
end

puts <<EOF
</div>

</body>
</html>
EOF
