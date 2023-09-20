package env

import "os"

func HOME() string {
	return os.Getenv("HOME")
}
