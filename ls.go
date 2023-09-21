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

		Run: func(args []Arg, env *Environment) (*Environment, error) {
			arg, err := GetArg(args, "[directory]")
			if err != nil {
				return nil, err
			}

			dir := filepath.Join(env.Pwd, arg.value)

			if fs.DirExists(dir) {
				// TODO(torbenconto): add support for -la and other flags, use seperate ls functions to list different types of files
				fs.ListDir(dir)
				return env, nil

			} else {
				return nil, errors.New("directory does not exist")
			}
		},
	}
}
