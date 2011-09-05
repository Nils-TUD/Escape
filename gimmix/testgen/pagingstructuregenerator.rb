class PagingStructureGenerator
	def initialize(name,segSizes,entries,pageSizeLog=13,root=0x2000,addressSpace=0)
		@name = name
		@segSizes = segSizes
		@entries = entries
		@pageSizeLog = pageSizeLog
		@root = root
		@addressSpace = addressSpace
		@ptpAddr = 0x400000
		@pteAddr = 0x200000
		@reg = 2
		@val = 0x1000
		@tests = ""
	end
	
	public
	def getHeader()
		pageSize = 1 << @pageSizeLog
		rv = (@segSizes[0] << 60) | (@segSizes[1] << 56) | (@segSizes[2] << 52) | (@segSizes[3] << 48)
		rv |= @pageSizeLog << 40
		rv |= @root
		rv |= @addressSpace << 3
		header = ""
		header += "%\n"
		header += "% #{@name}.mms -- tests paging (generated)\n"
		header += "%\n"
		header += "\n"
		header += sprintf("		LOC		#00000000\n")
		header += sprintf("		%% segmentsizes: %d,%d,%d,%d; pageSize=2^%d; r=%#x; n=%d\n",
			@segSizes[0],@segSizes[1],@segSizes[2],@segSizes[3],@pageSizeLog,@root,@addressSpace)
		header += sprintf("RV		OCTA	#%016X\n",rv)
		header += sprintf("\n")
	
		lastSeg = 0
		lastSize = -1
		for i in 0...@segSizes.length
			size = @segSizes[i] - lastSeg
			startAddr = i << 61
			if size == 0
				header += getSegStructures(pageSize,0,0,@root + lastSeg * 8 * 1024,startAddr,
					size,lastSize != size)
			else
				for j in 0...size
					mapSize = (1 << (10 * j)) * pageSize
					for k in 0...@entries.length
						header += getSegStructures(pageSize,j,@entries[k],
							@root + (lastSeg + j) * 8 * 1024,startAddr,size,true)
					end
				end
			end
			lastSize = size
			lastSeg = @segSizes[i]
			header += sprintf("\n")
		end
		return header
	end
	
	def getFooter()
		return <<FOOTER
		
		TRAP	0
FOOTER
	end
	
	def getTests()
		tests = <<TESTS
	
		LOC		#1000

Main	SETH	$0,#8000
		ORMH	$0,RV>>32
		ORML	$0,RV>>16
		ORL		$0,RV
		LDOU	$0,$0,0
		PUT		rV,$0

TESTS
		tests += @tests
		return tests
	end
	
	def getResults()
		total = 0
		lastSeg = 0
		for i in 0...@segSizes.length
			if @segSizes[i] - lastSeg == 0
				total += 1
			else
				total += (@segSizes[i] - lastSeg) * @entries.length
			end
			lastSeg = @segSizes[i]
		end
		results = ""
		results += sprintf("r:$2..$%d",total + 1)
		for i in 0...total
			results += sprintf("\n$%d: %016X",2 + i,@val)
			@val += 1
		end
		return results
	end
	
	private
	def getSegStructures(pageSize,layer,index,addr,mapAddr,segSize,genPTE)
		mapSize = (1 << (10 * layer)) * pageSize
		structures = ""
		if genPTE
			structures += sprintf("		LOC		#%08X\n",addr + 8 * index)
		end
		if layer == 0
			if genPTE
				structures += sprintf("		OCTA	#%016X	%% PTE  %-4d",
					@pteAddr | 0x7 | @addressSpace << 3,index)
			end
			loadAddr = mapAddr + mapSize * index
			@tests += sprintf("		SET		$0,#%04X\n",@val)
			@tests += sprintf("		SETH	$1,#%04X\n",(loadAddr >> 48) & 0xFFFF)
			@tests += sprintf("		ORMH	$1,#%04X\n",(loadAddr >> 32) & 0xFFFF)
			@tests += sprintf("		ORML	$1,#%04X\n",(loadAddr >> 16) & 0xFFFF)
			@tests += sprintf("		ORL		$1,#%04X\n",(loadAddr >>  0) & 0xFFFF)
			@tests += sprintf("		STOU	$0,$1,0		%% store %04X to %016X\n",@val,loadAddr)
			@tests += sprintf("		SETH	$1,#8000\n")
			@tests += sprintf("		ORMH	$1,#%04X\n",(@pteAddr >> 32) & 0xFFFF)
			@tests += sprintf("		ORML	$1,#%04X\n",(@pteAddr >> 16) & 0xFFFF)
			@tests += sprintf("		ORL		$1,#%04X\n",(@pteAddr >>  0) & 0xFFFF)
			@tests += sprintf("		LDOU	$%d,$1,0		%% load from %016X\n",
				@reg,@pteAddr | (1 << 63))
			@tests += "\n"
			@val += 1
			@reg += 1
			if segSize > 0
				@pteAddr += pageSize
			end
		else
			structures += sprintf("		OCTA	#%016X	%% PTP%d %-4d",
				(1 << 63) | @ptpAddr | @addressSpace << 3,layer,index)
			@ptpAddr += 1024 * 8
		end
		if genPTE
			structures += sprintf(" (#%016X .. #%016X)\n",mapAddr + mapSize * index,
				mapAddr + mapSize * (index + 1) - 1)
		end
		if layer > 0
			structures += getSegStructures(pageSize,layer - 1,0,@ptpAddr - 1024 * 8,
				mapAddr + mapSize * index,segSize,genPTE)
		end
		return structures
	end
end

