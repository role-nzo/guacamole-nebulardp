resource "azurerm_kubernetes_cluster" "hosts_aks" {
  name                = "cluster-hosts"
  location            = data.azurerm_resource_group.main.location
  resource_group_name = data.azurerm_resource_group.main.name
  dns_prefix          = "cluster-hosts"

  default_node_pool {
    name                = "system"
    node_count          = var.system_node_count
    vm_size             = "Standard_DS2_v2"
  }

  identity {
    type = "SystemAssigned"
  }

  network_profile {
    load_balancer_sku = "standard"
    network_plugin    = "kubenet"
    pod_cidr = "10.246.0.0/16"
    service_cidr = "10.2.0.0/16"
    dns_service_ip = "10.2.0.10"
  }
}