package main

import (
	"errors"
	"strings"
)

type Command struct {
	Name        string
	Description string
	Usage       string

	Args []Arg

	Run func(args []Arg) (string, error)
}

func NewCommand(name, description, usage string, args []Arg) *Command {
	return &Command{
		Name:        name,
		Description: description,
		Usage:       usage,
		Args:        args,
	}

}

type ArgMapKey struct {
	Name      string
	Shorthand string
}

func (c *Command) ArgsToMap() map[ArgMapKey]Arg {
	argmap := make(map[ArgMapKey]Arg)
	uniqueEntries := make(map[ArgMapKey]struct{})

	for _, arg := range c.Args {
		key := ArgMapKey{
			Name:      arg.Name,
			Shorthand: arg.Shorthand,
		}

		if _, exists := uniqueEntries[key]; !exists {
			argmap[key] = arg
			uniqueEntries[key] = struct{}{}
		}

		if arg.Name != "" {
			nameKey := ArgMapKey{
				Name: arg.Name,
			}

			if _, exists := uniqueEntries[nameKey]; !exists {
				argmap[nameKey] = arg
				uniqueEntries[nameKey] = struct{}{}
			}
		}

		if arg.Shorthand != "" {
			shorthandKey := ArgMapKey{
				Shorthand: arg.Shorthand,
			}

			if _, exists := uniqueEntries[shorthandKey]; !exists {
				argmap[shorthandKey] = arg
				uniqueEntries[shorthandKey] = struct{}{}
			}
		}
	}

	return argmap
}

var ErrInvalidArgument = errors.New("invalid argument: no data provided to non-flag argument")

func (c *Command) ExtractArgs(args []string) ([]Arg, error) {

	args = args[1:]

	argMap := c.ArgsToMap()
	var extractedArgs []Arg

	i := 0
	for i < len(args) {
		argName := strings.ReplaceAll(args[i], "-", "")

		var foundArg Arg
		var argValue string

		if arg, ok := argMap[ArgMapKey{Name: argName}]; ok {
			foundArg = arg

			if !arg.Flag && i+1 < len(args) {
				argValue = args[i+1]
				i++
			}
		} else {
			found := false
			for _, cmdArg := range c.Args {
				if cmdArg.Shorthand == argName {
					found = true
					foundArg = cmdArg

					if !cmdArg.Flag && i+1 < len(args) {
						argValue = args[i+1]
						i++
					}
					break
				}
			}

			if !found {
				return nil, errors.New("Invalid argument: " + argName)
			}
		}

		if argValue == "" && !foundArg.Flag {
			return nil, ErrInvalidArgument
		}

		extractedArgs = append(extractedArgs, Arg{Name: foundArg.Name, Shorthand: foundArg.Shorthand, Value: argValue, Flag: foundArg.Flag})
		i++
	}

	return extractedArgs, nil
}
