data "azurerm_resource_group" "main" {
  name = "srs2024-stu-g15"
}

output "id" {
  value = data.azurerm_resource_group.main.id
}
