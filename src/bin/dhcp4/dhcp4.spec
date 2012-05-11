{
  "module_spec": {
    "module_name": "dhcp4",
    "module_description": "DHCPv4 server daemon",
    "config_data": [
      { "item_name": "interface",
        "item_type": "string",
        "item_optional": false,
        "item_default": "eth0"
      }
    ],
    "commands": [
        {
            "command_name": "shutdown",
            "command_description": "Shut down DHCPv4 server",
            "command_args": []
        }
    ]
  }
}
