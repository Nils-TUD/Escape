#!/usr/bin/ruby -w

def isPrime(x)
	for y in 2...x
		if x % y == 0
			return false
		end
	end
	return true
end

startAddr = 0x100000
endAddr = 0x1001F8
printf("m:%012X..%012X\n",startAddr,endAddr - 8)
i = 2
while startAddr != endAddr
	if isPrime(i)
		if startAddr % 8 == 0
			if startAddr > 0x100000
				puts
			end
			printf("m[%016X]: ",startAddr)
		end
		printf("%04X",i)
		startAddr += 2
	end
	i += 1
end
