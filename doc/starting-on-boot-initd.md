## What is this?

After installing flaschen-taschen on your machine, you can start it on boot by creating an init.d script.

An example for that script lives under `init.d/flaschen-taschen-server`.

## Installation
1. Copy the script: `cp init.d/flaschen-taschen-server /etc/init.d/`
2. Make it executable: `chmod +x /etc/init.d/flaschen-taschen-server`
3. Add it to the boot sequence: `update-rc.d flaschen-taschen-server defaults`

## Usage - like any other service
1. Start the service: `service flaschen-taschen-server start`
2. Stop the service: `service flaschen-taschen-server stop`
3. Restart the service: `service flaschen-taschen-server restart`

## What about the demos?

The demos live under a separate repo, https://github.com/cgorringe/ft-demos

Instructions for running the demos are in that repo, including an init.d script for starting the demos on boot.
