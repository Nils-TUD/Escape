#!/usr/bin/ruby -w

FL_MMS	= 0x1
FL_TEST	= 0x2

@build = "build"
@testdir = "tests"
@tgens = "testgen"

tests = [
	["user/branch",		@tgens + "/genbranch.rb",	"",				FL_MMS | FL_TEST],
	["user/pbranch",	@tgens + "/genbranch.rb",	"--probable",	FL_MMS | FL_TEST],
	["user/cset",		@tgens + "/gencset.rb",		"",				FL_MMS | FL_TEST],
	["user/dif",		@tgens + "/gendif.rb",		"",				FL_MMS | FL_TEST],
	["user/load",		@tgens + "/genload.rb",		"",				FL_MMS | FL_TEST],
	["user/mor",		@tgens + "/genmor.rb",		"",				FL_MMS | FL_TEST],
	["user/sadd",		@tgens + "/gensadd.rb",		"",				FL_MMS | FL_TEST],
	["user/shift",		@tgens + "/genshift.rb",	"",				FL_MMS | FL_TEST],
	["user/store",		@tgens + "/genstore.rb",	"",				FL_MMS | FL_TEST],
	["user/zset",		@tgens + "/genzset.rb",		"",				FL_MMS | FL_TEST],
	["user/div",		@build + "/gendiv",			"--signed",		FL_MMS | FL_TEST],
	["user/divu",		@build + "/gendiv",			"",				FL_MMS | FL_TEST],
	["user/mul",		@tgens + "/genmul.rb",		"",				FL_MMS | FL_TEST],
	["user/mulu",		@tgens + "/genmul.rb",		"--unsigned",	FL_MMS | FL_TEST],
	["user/immround",	@tgens + "/genimmround.rb",	"",				FL_MMS | FL_TEST],
	["user/fdiv",		@build + "/genfdivtest",	"",				FL_MMS | FL_TEST],
	["user/fdiv-ex",	@build + "/genfdivtest",	"--ex",			FL_MMS | FL_TEST],
	["user/fmul",		@build + "/genfmultest",	"",				FL_MMS | FL_TEST],
	["user/fmul-ex",	@build + "/genfmultest",	"--ex",			FL_MMS | FL_TEST],
	["user/fadd",		@build + "/genfaddtest",	"fadd",			FL_MMS | FL_TEST],
	["user/fadd-ex",	@build + "/genfaddtest",	"fadd --ex",	FL_MMS | FL_TEST],
	["user/fsub",		@build + "/genfaddtest",	"fsub",			FL_MMS | FL_TEST],
	["user/fsub-ex",	@build + "/genfaddtest",	"fsub --ex",	FL_MMS | FL_TEST],
	["user/fcmp",		@build + "/genfcmptest",	"fcmp",			FL_MMS | FL_TEST],
	["user/fcmp-ex",	@build + "/genfcmptest",	"fcmp --ex",	FL_MMS | FL_TEST],
	["user/feql",		@build + "/genfcmptest",	"feql",			FL_MMS | FL_TEST],
	["user/feql-ex",	@build + "/genfcmptest",	"feql --ex",	FL_MMS | FL_TEST],
	["user/fun",		@build + "/genfcmptest",	"fun",			FL_MMS | FL_TEST],
	["user/fun-ex",		@build + "/genfcmptest",	"fun --ex",		FL_MMS | FL_TEST],
	["user/fcmpe",		@build + "/genfcmpetest",	"fcmpe",		FL_MMS | FL_TEST],
	["user/feqle",		@build + "/genfcmpetest",	"feqle",		FL_MMS | FL_TEST],
	["user/fune",		@build + "/genfcmpetest",	"fune",			FL_MMS | FL_TEST],
	["user/fune-ex",	@build + "/genfcmpetest",	"fune --ex",	FL_MMS | FL_TEST],
	["user/fint",		@build + "/genfinttest",	"fint",			FL_MMS | FL_TEST],
	["user/fint-ex",	@build + "/genfinttest",	"fint --ex",	FL_MMS | FL_TEST],
	["user/fix",		@build + "/genfinttest",	"fix",			FL_MMS | FL_TEST],
	["user/fix-ex",		@build + "/genfinttest",	"fix --ex",		FL_MMS | FL_TEST],
	["user/fixu",		@build + "/genfinttest",	"fixu",			FL_MMS | FL_TEST],
	["user/fixu-ex",	@build + "/genfinttest",	"fixu --ex",	FL_MMS | FL_TEST],
	["user/flot",		@build + "/genflottest",	"flot",			FL_MMS | FL_TEST],
	["user/flot-ex",	@build + "/genflottest",	"flot --ex",	FL_MMS | FL_TEST],
	["user/flotu",		@build + "/genflottest",	"flotu",		FL_MMS | FL_TEST],
	["user/flotu-ex",	@build + "/genflottest",	"flotu --ex",	FL_MMS | FL_TEST],
	["user/sflot",		@build + "/genflottest",	"sflot",		FL_MMS | FL_TEST],
	["user/sflot-ex",	@build + "/genflottest",	"sflot --ex",	FL_MMS | FL_TEST],
	["user/sflotu",		@build + "/genflottest",	"sflotu",		FL_MMS | FL_TEST],
	["user/sflotu-ex",	@build + "/genflottest",	"sflotu --ex",	FL_MMS | FL_TEST],
	["user/fsqrt",		@build + "/genfsqrttest",	"",				FL_MMS | FL_TEST],
	["user/fsqrt-ex",	@build + "/genfsqrttest",	"--ex",			FL_MMS | FL_TEST],
	["user/ldstsf",		@build + "/genldstsftest",	"",				FL_MMS | FL_TEST],
	["user/ldstsf-ex",	@build + "/genldstsftest",	"--ex",			FL_MMS | FL_TEST],
	["user/frem",		@build + "/genfremtest",	"",				FL_MMS | FL_TEST],
	["kernel/paging1",	@tgens + "/genpaging1.rb",	"",				FL_MMS | FL_TEST],
	["kernel/paging2",	@tgens + "/genpaging2.rb",	"",				FL_MMS | FL_TEST],
	["kernel/paging3",	@tgens + "/genpaging3.rb",	"",				FL_MMS | FL_TEST],
	["kernel/primegen",	@tgens + "/genprimegen.rb",	"",				FL_TEST],
	["kernel/resumecont",@tgens + "/genresumecont.rb","",			FL_MMS],
]

tests.each { |test|
	file,gen,args,flags = test[0],test[1],test[2],test[3]
	outfile = @testdir + "/" + file
	if (flags & FL_MMS) != 0
		cmd = "#{gen} mms #{args} > #{outfile}.mms"
		puts "Generating #{outfile}.mms..."
		if !system(cmd)
			$stderr.puts "Executing '" + cmd + "' failed"
			exit 1
		end
	end
	if (flags & FL_TEST) != 0
		cmd = "#{gen} test #{args} > #{outfile}.test"
		puts "Generating #{outfile}.test..."
		if !system(cmd)
			$stderr.puts "Executing '" + cmd + "' failed"
			exit 1
		end
	end
}

