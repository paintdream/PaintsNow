local Event = record
	type : string
	stackable : boolean
	data : any
	_next : Event
end

return Event