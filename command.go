package ares

import (
	"errors"
	"strings"
)

type Command struct {
	Name        string
	Description string
	Usage       string

	env *Environment

	Args []Arg

	Run func(args []Arg, env *Environment) (string, *Environment, error)
}

func NewCommand(name, description, usage string, args []Arg) *Command {
	return &Command{
		Name:        name,
		Description: description,
		Usage:       usage,
		Args:        args,
	}

}

func GetArg(in []Arg, target string) (Arg, error) {
	for _, arg := range in {
		if arg.Name == target {
			return arg, nil
		}
	}

	return Arg{}, errors.New("cannot get argument: " + target)
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

// When using multiple param arguments, the n-ordened params in "args" sometimes randomly get switched
func (c *Command) ExtractArgs(args []string) ([]Arg, error) {
	args = args[1:]

	argMap := c.ArgsToMap()
	var extractedArgs []Arg

	paramsMap := make(map[int]Arg)

	i := 0
	for i < len(c.Args) {
		if c.Args[i].Param {
			paramsMap[i+1] = c.Args[i]
		}

		i++
	}

	default_count := 0
	for k, _ := range paramsMap {
		if paramsMap[k].Default_value != "" {
			args = append(args, paramsMap[k].Default_value)
			default_count++
		}
	}

	j := 0
	for j < len(args) {
		argName := strings.ReplaceAll(args[j], "-", "")

		var foundArg Arg
		var argValue string

		paramValue := paramsMap[j+1]

		if paramValue.Param {

			if len(args) != default_count && len(args) != 0 {
				args = args[:len(args)-default_count]
			}

			argValue = args[j]
			extractedArgs = append(extractedArgs, Arg{Name: paramValue.Name, Shorthand: paramValue.Shorthand, value: argValue, Flag: paramValue.Flag})
			j++
			continue
		}

		if arg, ok := argMap[ArgMapKey{Name: argName}]; ok {
			foundArg = arg

			if !arg.Flag && j+1 < len(args) {
				argValue = args[j+1]
				j++
			}
		} else {
			found := false
			for _, cmdArg := range c.Args {
				if cmdArg.Shorthand == argName {
					found = true
					foundArg = cmdArg

					if !cmdArg.Flag && !cmdArg.Param && j+1 < len(args) {
						argValue = args[j+1]
						j++
					}
					break
				}
			}

			if !found {
				return nil, errors.New("Invalid argument: " + args[i])
			}
		}

		if argValue == "" && !foundArg.Flag {
			return nil, ErrInvalidArgument
		}

		extractedArgs = append(extractedArgs, Arg{Name: foundArg.Name, Shorthand: foundArg.Shorthand, value: argValue, Flag: foundArg.Flag})
		j++
	}

	return extractedArgs, nil
}
