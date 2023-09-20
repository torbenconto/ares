package main

import "fmt"

var Commands_Builtin = []Command{
	Command{
		Name: "help",

		Args: []Arg{
			Arg{
				Name:      "cmd",
				Shorthand: "c",
				Flag:      true,
			},
		},
	},
}

func CommandsToMap(commands []Command) map[string]Command {

	commandmap := make(map[string]Command)

	for _, c := range commands {
		commandmap[c.Name] = c
	}

	return commandmap
}

func IdentifyCommand(in string) (Command, error) {
	commandmap := CommandsToMap(Commands_Builtin)

	if commandmap[in].Args != nil {
		return commandmap[in], nil
	}

	return Command{}, fmt.Errorf("unknown command")
}
