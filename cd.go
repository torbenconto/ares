package ares

import (
	"errors"
	"path/filepath"

	"github.com/torbenconto/ares/pkg/fs"
)

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
			arg, err := GetArg(args, "[directory]")
			if err != nil {
				return "", nil, err
			}

			dir := filepath.Join(env.Pwd, arg.value)

			if fs.DirExists(dir) {
				env.Pwd = filepath.Join(env.Pwd, arg.value)
				return "", env, nil
			} else {
				return "", nil, errors.New("directory does not exist")
			}
		},
	}
}
