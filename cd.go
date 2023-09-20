package ares

import "path/filepath"

func Command_cd() Command {
	return Command{
		Name:        "cd",
		Description: "Change the shell working directory.",
		Usage:       "cd [directory]",

		Args: []Arg{
			Arg{
				Name:  "[directory]",
				Param: true,
			},
		},

		Run: func(args []Arg, env *Environment) (string, *Environment, error) {
			env.Pwd = filepath.Join(env.Pwd, args[0].value)
			return env.Pwd, env, nil
		},
	}
}
