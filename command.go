package main

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
