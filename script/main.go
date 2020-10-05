package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"strings"
	"time"
)

// ENDPOINT is the base URL that the webserver is hosted on
const ENDPOINT = "http://192.168.68.220"

var client = &http.Client{Timeout: 10 * time.Second}

// Node represents a single switch that can be toggled (ex. light, fan)
type Node struct {
	Name  string `json:"name"`
	State string `json:"state"`
}

func main() {
	if len(os.Args) < 2 { // info
		fmt.Println("welcome to Automator")
		os.Exit(0)
	}
	command := os.Args[1]
	if command == "get" {
		res, err := client.Get(fmt.Sprintf("%s/get", ENDPOINT))
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		defer res.Body.Close()
		var nodes []Node
		json.NewDecoder(res.Body).Decode(&nodes)

		for _, node := range nodes {
			if len(os.Args) < 3 || node.Name == strings.ToLower(os.Args[2]) {
				fmt.Print(node.Name + ": ")
				if node.State == "1" {
					fmt.Println("ON")
				} else {
					fmt.Println("OFF")
				}
			}
		}
	} else if command == "set" {
		if len(os.Args) < 4 {
			fmt.Println("specify a node and a state")
			os.Exit(1)
		}

		var state string
		if strings.ToUpper(os.Args[3]) == "ON" || os.Args[3] == "1" {
			state = "1"
		} else if strings.ToUpper(os.Args[3]) == "OFF" || os.Args[3] == "0" {
			state = "0"
		} else {
			fmt.Println("invalid state")
			os.Exit(1)
		}
		node := Node{
			Name:  strings.ToLower(os.Args[2]),
			State: state,
		}

		url := fmt.Sprintf("%s/set?%s=%s", ENDPOINT, node.Name, node.State)
		res, err := client.Post(url, "application/json", new(bytes.Buffer))
		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}
		resText, err := ioutil.ReadAll(res.Body)
		if string(resText) == "null" {
			fmt.Println("invalid node")
			os.Exit(1)
		} else {
			fmt.Print(node.Name + ": ")
			if node.State == "1" {
				fmt.Println("ON")
			} else {
				fmt.Println("OFF")
			}
		}
		os.Exit(0)
	} else {
		fmt.Println("command must be either \"get\" or \"set\"")
		os.Exit(1)
	}
}
