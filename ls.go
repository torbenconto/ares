package ares

import (
	"errors"
	"path/filepath"

	"github.com/torbenconto/ares/pkg/fs"
)

func Command_ls() Command {
	return Command{
		Name:        "ls",
		Description: "List information about the FILEs (the current directory by default.",
		Usage:       "ls [directory] [flags]",

		Args: []Arg{
			Arg{
				Name:          "[directory]",
				Default_value: ".",
				Param:         true,
			},
		},

		Run: func(args []Arg, env *Environment) (string, *Environment, error) {
			arg, err := GetArg(args, "[directory]")
			if err != nil {
				return "", nil, err
			}

			dir := filepath.Join(env.Pwd, arg.value)

			if fs.DirExists(dir) {
				env.Pwd = filepath.Join(env.Pwd, arg.value)
				return env.Pwd, env, nil
			} else {
				return "", nil, errors.New("directory does not exist")
			}
		},
	}
}
