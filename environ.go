package ares

import (
	"strings"

	"github.com/torbenconto/ares/pkg/env"
)

type Environment struct {
	Pwd string
}

func NewEnvironment() *Environment {
	return &Environment{
		Pwd: env.HOME(),
	}
}

func (e *Environment) Eval(in string) (string, *Environment, error) {
	strargs := strings.Split(in, " ")

	command, err := IdentifyCommand(strargs[0])
	if err != nil {
		return "", e, err
	}

	if command.Args != nil {
		args, err := command.ExtractArgs(strargs)
		if err != nil {
			return "", e, err
		}

		return command.Run(args, e)
	}
	return command.Run([]Arg{}, e)
}
