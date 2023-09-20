package main

type Arg struct {
	Name      string
	Shorthand string
	Value     string

	Flag bool
}

func NewArg(name, shorthand string, flag bool) *Arg {
	return &Arg{Name: name, Shorthand: shorthand, Flag: flag}
}
