#
# Component Makefile for ESP Service Discovery
#

COMPONENT_SRCDIRS := .
COMPONENT_ADD_INCLUDEDIRS := include

COMPONENT_DEPENDS := mdns esp_wifi esp_netif esp_event nvs_flash
