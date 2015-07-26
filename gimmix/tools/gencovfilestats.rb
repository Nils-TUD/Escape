#!/usr/bin/ruby -w
total = 0
unseen = 0
seen = 0
begin
	file = File.new(ARGV[0], "r")
	while (line = file.gets)
		line.scan(/^\W*(\d+|\-|\#+):\W*(\d+):(.*)$/) { |cov,no,rem|
			if no.to_i > 0
				total += 1
				if cov == "#"
					unseen += 1
				elsif cov != "-"
					seen += 1
				end
			end
		}
	end
	file.close
rescue => err
	puts "Exception: #{err}"
	err
end

puts "total  =" + total.to_s
puts "unseen =" + unseen.to_s
puts "seen   =" + seen.to_s

