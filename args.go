package ares

type Arg struct {
	Name      string
	Shorthand string

	value string

	Param bool
	Flag  bool
}

func NewArg(name, shorthand string, flag bool) *Arg {
	return &Arg{Name: name, Shorthand: shorthand, Flag: flag}
}
