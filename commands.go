package ares

import (
	"fmt"
)

var cd = Command_cd()

var Commands_Builtin = []Command{
	cd,
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

	if commandmap[in].Name != "" {
		return commandmap[in], nil
	}

	return Command{}, fmt.Errorf("unknown command")
}
