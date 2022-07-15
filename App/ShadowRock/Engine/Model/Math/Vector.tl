local Vector = record
	{ number }
	__index : any
end

Vector.__index = Vector

function Vector.Attach(vec : { number }) : Vector
	return setmetatable(vec, Vector as METATABLE) as Vector
end

function Vector.New(x : number, y : number, z : number) : Vector
	return setmetatable({ x, y, z }, Vector as METATABLE) as Vector
end

function Vector.__add(a : Vector, b : Vector) : Vector
	return Vector.New(a[1] + b[1], a[2] + b[2], a[3] + b[3])
end

function Vector.__sub(a : Vector, b : Vector) : Vector
	return Vector.New(a[1] - b[1], a[2] - b[2], a[3] - b[3])
end

function Vector.__mul(a : Vector, b : Vector | number) : Vector
	if type(b) == "number" then
		return Vector.New(a[1] * (b as number), a[2] * (b as number), a[3] * (b as number))
	else
		return Vector.New(a[1] * (b as Vector)[1], a[2] * (b as Vector)[2], a[3] * (b as Vector)[3])
	end
end

function Vector.__div(a : Vector, b : Vector | number) : Vector
	if type(b) == "number" then
		local bb = 1.0 / b as number
		return Vector.New(a[1] * bb, a[2] * bb, a[3] * bb)
	else
		return Vector.New(a[1] / (b as Vector)[1], a[2] / (b as Vector)[2], a[3] / (b as Vector)[3])
	end
end

function Vector.Dot(a : Vector, b : Vector) : number
	return a[1] * b[1] + a[2] * b[2] + a[3] * b[3]
end

function Vector:SquareLength() : number
	return Vector.Dot(self, self)
end

function Vector:Length() : number
	return math.sqrt(self:SquareLength())
end

function Vector:Normalized() : Vector
	return Vector.__div(self, self:Length())
end

function Vector.Cross2D(a : Vector, b : Vector) : number
	return a[1] * b[2] - a[2] * b[1]
end

function Vector.Cross3D(a : Vector, b : Vector) : Vector
	return Vector.New(a[2] * b[3] - a[3] * b[2], a[3] * b[1] - b[3] * a[1], a[1] * b[2] - a[2] * b[1])
end

function Vector:__tostring() : string
	return "{ " .. table.concat(self as { string }, ", ") .. " }"
end

return Vector