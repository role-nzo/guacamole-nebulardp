resource "azurerm_kubernetes_cluster" "guaca_aks" {
  name                = "cluster-guaca"
  location            = data.azurerm_resource_group.main.location
  resource_group_name = data.azurerm_resource_group.main.name
  dns_prefix          = "cluster-guaca"

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
    pod_cidr = "10.245.0.0/16"
    service_cidr = "10.1.0.0/16"
    dns_service_ip = "10.1.0.10"
  }
}