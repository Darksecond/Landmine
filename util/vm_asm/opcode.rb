class Opcode
	def self.min(value)
		send :define_method, :min do
			value
		end
	end

	def self.max(value)
		send :define_method, :max do
			value
		end
	end

	def self.handle(*args)
		@@handles ||= {}
		args.each do |raw|
			@@handles[raw] = self
		end
	end

	def self.from_raw(raw, *args)
		@@handles.fetch(raw).new(*args)
	end

	def initialize(*args)
		@args = args
	end
	
	def arguments
		@args ||= []
	end

	def matcher
		arguments.map do |a|
			a.class
		end
	end
	
	def assemble
		if arguments.count < min || arguments.count > max
			raise "argument count out of range"
		end

		output = [nil]
		arguments.each do |arg|
			output += arg.assemble
		end

		output[0] = opcode
		if output[0] == nil
			raise "opcode invalid"
		end
		output
	end
end

class MOV < Opcode
	handle "mov"
	min 2
	max 2

	def opcode
		case matcher
		when [Register, Register]
			0x20
		when [Register, Pointer]
			0x21
		when [Pointer, Register]
			0x22
		when [Register, Short]
			0x23
		when [Register, Long]
			0x48
		else
			nil
		end
	end
end

class Register
	def initialize(register)
		@reg = register.to_i
	end
	
	def assemble
		[@reg-1]
	end
end

class Pointer
	def initialize(value)
		@value = value
	end

	def assemble
		@value.assemble
	end

end

class Short
	def initialize(value)
		@value = value.to_i
	end

	def assemble
		[(@value & 0xff)]
	end
end

class Long
	def initialize(value)
		@value = value.to_i
	end

	def assemble
		[((@value >> 8 ) & 0xff), (@value & 0xff)]
	end
end

class Program
	def self.from_raw(raw)
		prog = Program.new
		raw.split("\n").each do |line|
			split = line.split(" ")
			raw_opcode = split.first
			raw_arguments = split[1..split.count-1].join.split(",")
			#parse arguments!
			arguments = []
			raw_arguments.each do |arg|
				arguments << parse_argument(arg)
			end
			opcode = Opcode.from_raw(raw_opcode, *arguments)
			prog.prog << opcode
		end
		prog
	end

	def self.parse_argument(arg)
		begin
			num = Float(arg).to_i
		rescue
			num = nil
		end
		if arg[0] == "r"
			Register.new(arg[1..-1])
		elsif arg[0] == "["
			Pointer.new(parse_argument(arg[1..-2]))
		elsif num
			#we need to see if num is a short, or a long
			if (num & 0xff) == num
				Short.new(num)
			else
				Long.new(num)
			end
		else
			raise "argument invalid"
		end
	end

	def initialize()
		@prog = []
	end

	def prog
		@prog
	end

	def assemble
		prog.map do |opcode|
			opcode.assemble
		end.flatten
	end
end
