class Opcode
	def self.handle(*args)
		@@handles ||= {}
		args.each do |raw|
			@@handles[raw] = self
		end
	end

	def self.from_raw(raw, *args)
		@@handles.fetch(raw).new(*args)
	end

	def self.opcode_matcher(opcode, matcher)
		@opcodes ||= {}
		@opcodes[matcher] = opcode
	end

	def self.get_opcode(arguments)
		matcher = arguments.map { |a| a.type }
		@opcodes.fetch matcher
	end

	def self.sizes
		sizes = @opcodes.keys.map { |m| m.count }
		[sizes.min, sizes.max]
	end

	attr_accessor :location

	def min
		self.class.sizes[0]
	end
	
	def max
		self.class.sizes[1]
	end

	def initialize(*args)
		@args = args
	end
	
	def arguments
		@args ||= []
	end

	def opcode
		self.class.get_opcode(arguments)
	end
	
	def assemble
		if arguments.count < min || arguments.count > max
			raise "argument count out of range"
		end

		output = [opcode]
		if output[0] == nil
			raise "opcode invalid"
		end

		arguments.inject(output) { |out, arg| out << arg.assemble }
	end

	def length
		return 1 + arguments.inject(0) { |sum, arg| sum + arg.length }
	end
end

class Argument
	def self.handle(&block)
		@@handles ||= []
		@@handles << block
	end

	def self.length(length = nil, &block)
		if block == nil
			send :define_method, :length do
				length
			end
		else
			send :define_method, :length, block
		end
	end

	def self.parse_argument(arg, prog)
		argument = @@handles.map { |handle| handle.call(arg.strip, prog) }.compact.first
		if argument.class == nil
			raise "argument invalid: #{arg}"
		else
			argument
		end
	end

	def self.parse_arguments(args, prog)
		args.map do |arg|
			parse_argument(arg, prog)
		end
	end

	def initialize(value, prog=nil)
		@prog = prog
		@value = value
	end

	def type
		self.class
	end
end

class Register < Argument
	handle do |argument|
		if argument[0] == "r"
			Register.new argument[1..-1].to_i
		end
	end
	length 1
	
	def assemble
		[@value-1]
	end
end

class Pointer < Argument
	handle do |argument, prog|
		if argument[0] == "["
			Pointer.new Argument.parse_argument(argument[1..-2], prog)
		end
	end

	length do
		@value.length
	end

	def assemble
		@value.assemble
	end
end

class Short < Argument
	handle do |argument|
		begin
			num = Float(argument).to_i
			if (num & 0xff) == num
				Short.new num
			end
		rescue
			nil
		end
	end
	
	length 1

	def assemble
		[(@value & 0xff)]
	end
end

class Long < Argument
	handle do |argument|
		begin
			num = Float(argument).to_i
			Long.new num
		rescue
			nil
		end
	end

	length 2

	def assemble
		[((@value >> 8 ) & 0xff), (@value & 0xff)]
	end
end

class Reference < Argument
	handle do |argument, prog|
		if argument[0] == "@"
			Reference.new argument[1..-1], prog
		end
	end

	length 2

	def assemble
		begin
			num = Float(@value).to_i
			upper = (num >> 8) & 0xff
			lower = num & 0xff
		rescue
			opcode = @prog.label @value
			upper = (opcode.location >> 8) & 0xff
			lower = opcode.location & 0xff
		end
		[upper, lower]
	end
end

class MOV < Opcode
	handle "mov"
	opcode_matcher 0x20, [Register, Register]
	opcode_matcher 0x21, [Register, Pointer]
	opcode_matcher 0x22, [Pointer, Register]
	opcode_matcher 0x23, [Register, Short]
	opcode_matcher 0x48, [Register, Long]
end

class Program
	def self.from_raw(raw)
		Program.new raw
	end
	
	def self.read(filename)
		Program.new File.open filename
	end

	attr_reader :prog

	def initialize(prog)
		if prog.class == Array
			@prog = prog
		else
			@prog = parse(prog)
		end
	end

	def parse(raw)
		if raw.class == String
			raw = raw.split("\n")
		end

		raw.map { |line| parse_line line }.flatten.compact
	end

	def parse_line(line)
		line = line.strip
		if line[-1] == ":" #label
			@label = line[0..-2]
			nil
		else
			split = line.split(" ")
			arguments = Argument.parse_arguments(split[1..-1].join.split(","), self)
			opcode = Opcode.from_raw(split.first.strip, *arguments)
			if @label
				labels[@label] = opcode
				@label = nil
			end
			opcode
		end
	end

	def labels
		@labels ||= {}
	end

	def label(label)
		labels.fetch label
	end

	def pass1
		cur = 0;
		prog.each { |opcode|
			opcode.location = cur
			cur += opcode.length
		}
	end

	def pass2
		prog.map { |opcode| opcode.assemble }.flatten
	end
	
	def assemble
		pass1
		pass2
	end

	def write(filename)
		File.open filename, "wb" do |file|
			assemble.each do |byte|
				file.print byte.chr
			end
		end
	end
end
