variable "location" {
  type = string
}
/*
variable "cacert" {
  type = string
  default = <<EOH
  -----BEGIN NEBULA CERTIFICATE-----
CkUKE015b3JnYW5pemF0aW9uLCBJbmMoucb4sQYwua39wAY6II45Y5vHkqpartJ3
NCm/RQIE9ov988ikM9HWKj9t/rZhQAESQEn7mIGasjI9DAtZWBVHLTGfNFMy3p1i
EWpl7PEG8He9MuAlL9l9GCiMMfX0gLJYSkQBiC+Pu8hqGx2FQax8QQM=
-----END NEBULA CERTIFICATE-----
EOH
}*/

variable "system_node_count" {
  type = string
  default = 2
}