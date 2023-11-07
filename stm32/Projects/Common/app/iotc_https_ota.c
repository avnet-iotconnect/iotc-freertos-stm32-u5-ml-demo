#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "logging_levels.h"
/* define LOG_LEVEL here if you want to modify the logging level from the default */

#define LOG_LEVEL LOG_INFO

#include "logging.h"

#include "FreeRTOS.h"
#include "mbedtls_transport.h"
#include "core_http_client.h"
#include "ota_pal.h"

#define IOTCONNECT_BALTIMORE_CYBER_TRUST_ROOT \
"-----BEGIN CERTIFICATE-----\n"\
"MIIFWjCCBEKgAwIBAgIQDxSWXyAgaZlP1ceseIlB4jANBgkqhkiG9w0BAQsFADBa\n"\
"MQswCQYDVQQGEwJJRTESMBAGA1UEChMJQmFsdGltb3JlMRMwEQYDVQQLEwpDeWJl\n"\
"clRydXN0MSIwIAYDVQQDExlCYWx0aW1vcmUgQ3liZXJUcnVzdCBSb290MB4XDTIw\n"\
"MDcyMTIzMDAwMFoXDTI0MTAwODA3MDAwMFowTzELMAkGA1UEBhMCVVMxHjAcBgNV\n"\
"BAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEgMB4GA1UEAxMXTWljcm9zb2Z0IFJT\n"\
"QSBUTFMgQ0EgMDEwggIiMA0GCSqGSIb3DQEBAQUAA4ICDwAwggIKAoICAQCqYnfP\n"\
"mmOyBoTzkDb0mfMUUavqlQo7Rgb9EUEf/lsGWMk4bgj8T0RIzTqk970eouKVuL5R\n"\
"IMW/snBjXXgMQ8ApzWRJCZbar879BV8rKpHoAW4uGJssnNABf2n17j9TiFy6BWy+\n"\
"IhVnFILyLNK+W2M3zK9gheiWa2uACKhuvgCca5Vw/OQYErEdG7LBEzFnMzTmJcli\n"\
"W1iCdXby/vI/OxbfqkKD4zJtm45DJvC9Dh+hpzqvLMiK5uo/+aXSJY+SqhoIEpz+\n"\
"rErHw+uAlKuHFtEjSeeku8eR3+Z5ND9BSqc6JtLqb0bjOHPm5dSRrgt4nnil75bj\n"\
"c9j3lWXpBb9PXP9Sp/nPCK+nTQmZwHGjUnqlO9ebAVQD47ZisFonnDAmjrZNVqEX\n"\
"F3p7laEHrFMxttYuD81BdOzxAbL9Rb/8MeFGQjE2Qx65qgVfhH+RsYuuD9dUw/3w\n"\
"ZAhq05yO6nk07AM9c+AbNtRoEcdZcLCHfMDcbkXKNs5DJncCqXAN6LhXVERCw/us\n"\
"G2MmCMLSIx9/kwt8bwhUmitOXc6fpT7SmFvRAtvxg84wUkg4Y/Gx++0j0z6StSeN\n"\
"0EJz150jaHG6WV4HUqaWTb98Tm90IgXAU4AW2GBOlzFPiU5IY9jt+eXC2Q6yC/Zp\n"\
"TL1LAcnL3Qa/OgLrHN0wiw1KFGD51WRPQ0Sh7QIDAQABo4IBJTCCASEwHQYDVR0O\n"\
"BBYEFLV2DDARzseSQk1Mx1wsyKkM6AtkMB8GA1UdIwQYMBaAFOWdWTCCR1jMrPoI\n"\
"VDaGezq1BE3wMA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYI\n"\
"KwYBBQUHAwIwEgYDVR0TAQH/BAgwBgEB/wIBADA0BggrBgEFBQcBAQQoMCYwJAYI\n"\
"KwYBBQUHMAGGGGh0dHA6Ly9vY3NwLmRpZ2ljZXJ0LmNvbTA6BgNVHR8EMzAxMC+g\n"\
"LaArhilodHRwOi8vY3JsMy5kaWdpY2VydC5jb20vT21uaXJvb3QyMDI1LmNybDAq\n"\
"BgNVHSAEIzAhMAgGBmeBDAECATAIBgZngQwBAgIwCwYJKwYBBAGCNyoBMA0GCSqG\n"\
"SIb3DQEBCwUAA4IBAQCfK76SZ1vae4qt6P+dTQUO7bYNFUHR5hXcA2D59CJWnEj5\n"\
"na7aKzyowKvQupW4yMH9fGNxtsh6iJswRqOOfZYC4/giBO/gNsBvwr8uDW7t1nYo\n"\
"DYGHPpvnpxCM2mYfQFHq576/TmeYu1RZY29C4w8xYBlkAA8mDJfRhMCmehk7cN5F\n"\
"JtyWRj2cZj/hOoI45TYDBChXpOlLZKIYiG1giY16vhCRi6zmPzEwv+tk156N6cGS\n"\
"Vm44jTQ/rs1sa0JSYjzUaYngoFdZC4OfxnIkQvUIA4TOFmPzNPEFdjcZsgbeEz4T\n"\
"cGHTBPK4R28F44qIMCtHRV55VMX53ev6P3hRddJb\n"\
"-----END CERTIFICATE-----\n"

#define IOTCONNECT_DIGICERT_GLOBAL_ROOT_G2 \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"\
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"\
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"\
"MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"\
"MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"\
"b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"\
"9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"\
"2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"\
"1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"\
"q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"\
"tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"\
"vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"\
"BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"\
"5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"\
"1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"\
"NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"\
"Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"\
"8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"\
"pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"\
"MrY=\n"\
"-----END CERTIFICATE-----\n"

// CN = Go Daddy Root Certificate Authority - G2
#define GODADDY_ROOT_CERTIFICATE_AUTHORITY_G2 \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDxTCCAq2gAwIBAgIBADANBgkqhkiG9w0BAQsFADCBgzELMAkGA1UEBhMCVVMx\n"\
"EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxGjAYBgNVBAoT\n"\
"EUdvRGFkZHkuY29tLCBJbmMuMTEwLwYDVQQDEyhHbyBEYWRkeSBSb290IENlcnRp\n"\
"ZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5MDkwMTAwMDAwMFoXDTM3MTIzMTIz\n"\
"NTk1OVowgYMxCzAJBgNVBAYTAlVTMRAwDgYDVQQIEwdBcml6b25hMRMwEQYDVQQH\n"\
"EwpTY290dHNkYWxlMRowGAYDVQQKExFHb0RhZGR5LmNvbSwgSW5jLjExMC8GA1UE\n"\
"AxMoR28gRGFkZHkgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIw\n"\
"DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAL9xYgjx+lk09xvJGKP3gElY6SKD\n"\
"E6bFIEMBO4Tx5oVJnyfq9oQbTqC023CYxzIBsQU+B07u9PpPL1kwIuerGVZr4oAH\n"\
"/PMWdYA5UXvl+TW2dE6pjYIT5LY/qQOD+qK+ihVqf94Lw7YZFAXK6sOoBJQ7Rnwy\n"\
"DfMAZiLIjWltNowRGLfTshxgtDj6AozO091GB94KPutdfMh8+7ArU6SSYmlRJQVh\n"\
"GkSBjCypQ5Yj36w6gZoOKcUcqeldHraenjAKOc7xiID7S13MMuyFYkMlNAJWJwGR\n"\
"tDtwKj9useiciAF9n9T521NtYJ2/LOdYq7hfRvzOxBsDPAnrSTFcaUaz4EcCAwEA\n"\
"AaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYE\n"\
"FDqahQcQZyi27/a9BUFuIMGU2g/eMA0GCSqGSIb3DQEBCwUAA4IBAQCZ21151fmX\n"\
"WWcDYfF+OwYxdS2hII5PZYe096acvNjpL9DbWu7PdIxztDhC2gV7+AJ1uP2lsdeu\n"\
"9tfeE8tTEH6KRtGX+rcuKxGrkLAngPnon1rpN5+r5N9ss4UXnT3ZJE95kTXWXwTr\n"\
"gIOrmgIttRD02JDHBHNA7XIloKmf7J6raBKZV8aPEjoJpL1E/QYVN8Gb5DKj7Tjo\n"\
"2GTzLH4U/ALqn83/B2gX2yKQOC16jdFU8WnjXzPKej17CuPKf1855eJ1usV2GDPO\n"\
"LPAvTK33sefOT6jEm0pUBsV/fdUID+Ic/n4XuKxe9tQWskMJDE32p2u0mYRlynqI\n"\
"4uJEvlz36hz1\n"\
"-----END CERTIFICATE-----\n"

// Starfield Services Root Certificate Authority - G2
#define STARFIELD_ROOT_CA_G2 \
"-----BEGIN CERTIFICATE-----\n"\
"MIID7zCCAtegAwIBAgIBADANBgkqhkiG9w0BAQsFADCBmDELMAkGA1UEBhMCVVMx\n"\
"EDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNjb3R0c2RhbGUxJTAjBgNVBAoT\n"\
"HFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xOzA5BgNVBAMTMlN0YXJmaWVs\n"\
"ZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1dGhvcml0eSAtIEcyMB4XDTA5\n"\
"MDkwMTAwMDAwMFoXDTM3MTIzMTIzNTk1OVowgZgxCzAJBgNVBAYTAlVTMRAwDgYD\n"\
"VQQIEwdBcml6b25hMRMwEQYDVQQHEwpTY290dHNkYWxlMSUwIwYDVQQKExxTdGFy\n"\
"ZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTswOQYDVQQDEzJTdGFyZmllbGQgU2Vy\n"\
"dmljZXMgUm9vdCBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkgLSBHMjCCASIwDQYJKoZI\n"\
"hvcNAQEBBQADggEPADCCAQoCggEBANUMOsQq+U7i9b4Zl1+OiFOxHz/Lz58gE20p\n"\
"OsgPfTz3a3Y4Y9k2YKibXlwAgLIvWX/2h/klQ4bnaRtSmpDhcePYLQ1Ob/bISdm2\n"\
"8xpWriu2dBTrz/sm4xq6HZYuajtYlIlHVv8loJNwU4PahHQUw2eeBGg6345AWh1K\n"\
"Ts9DkTvnVtYAcMtS7nt9rjrnvDH5RfbCYM8TWQIrgMw0R9+53pBlbQLPLJGmpufe\n"\
"hRhJfGZOozptqbXuNC66DQO4M99H67FrjSXZm86B0UVGMpZwh94CDklDhbZsc7tk\n"\
"6mFBrMnUVN+HL8cisibMn1lUaJ/8viovxFUcdUBgF4UCVTmLfwUCAwEAAaNCMEAw\n"\
"DwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAQYwHQYDVR0OBBYEFJxfAN+q\n"\
"AdcwKziIorhtSpzyEZGDMA0GCSqGSIb3DQEBCwUAA4IBAQBLNqaEd2ndOxmfZyMI\n"\
"bw5hyf2E3F/YNoHN2BtBLZ9g3ccaaNnRbobhiCPPE95Dz+I0swSdHynVv/heyNXB\n"\
"ve6SbzJ08pGCL72CQnqtKrcgfU28elUSwhXqvfdqlS5sdJ/PHLTyxQGjhdByPq1z\n"\
"qwubdQxtRbeOlKyWN7Wg0I8VRw7j6IPdj/3vQQF3zCepYoUz8jcI73HPdwbeyBkd\n"\
"iEDPfUYd/x7H4c7/I9vG+o1VTqkC50cRRj70/b17KSa7qWFiNyi2LSr2EIZkyXCn\n"\
"0q23KXB56jzaYyWf/Wi3MOxw+3WKt21gZ7IeyLnp2KhvAotnDU0mV3HaIPzBSlCN\n"\
"sSi6\n"\
"-----END CERTIFICATE-----"

#if 0
// Starfield Services Root Certificate Authority - G2
#define STARFIELD_ROOT_CA_G2 \
"-----BEGIN CERTIFICATE-----\n"\
"MIIEdTCCA12gAwIBAgIJAKcOSkw0grd/MA0GCSqGSIb3DQEBCwUAMGgxCzAJBgNV\n"\
"BAYTAlVTMSUwIwYDVQQKExxTdGFyZmllbGQgVGVjaG5vbG9naWVzLCBJbmMuMTIw\n"\
"MAYDVQQLEylTdGFyZmllbGQgQ2xhc3MgMiBDZXJ0aWZpY2F0aW9uIEF1dGhvcml0\n"\
"eTAeFw0wOTA5MDIwMDAwMDBaFw0zNDA2MjgxNzM5MTZaMIGYMQswCQYDVQQGEwJV\n"\
"UzEQMA4GA1UECBMHQXJpem9uYTETMBEGA1UEBxMKU2NvdHRzZGFsZTElMCMGA1UE\n"\
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjE7MDkGA1UEAxMyU3RhcmZp\n"\
"ZWxkIFNlcnZpY2VzIFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5IC0gRzIwggEi\n"\
"MA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDVDDrEKvlO4vW+GZdfjohTsR8/\n"\
"y8+fIBNtKTrID30892t2OGPZNmCom15cAICyL1l/9of5JUOG52kbUpqQ4XHj2C0N\n"\
"Tm/2yEnZtvMaVq4rtnQU68/7JuMauh2WLmo7WJSJR1b/JaCTcFOD2oR0FMNnngRo\n"\
"Ot+OQFodSk7PQ5E751bWAHDLUu57fa4657wx+UX2wmDPE1kCK4DMNEffud6QZW0C\n"\
"zyyRpqbn3oUYSXxmTqM6bam17jQuug0DuDPfR+uxa40l2ZvOgdFFRjKWcIfeAg5J\n"\
"Q4W2bHO7ZOphQazJ1FTfhy/HIrImzJ9ZVGif/L4qL8RVHHVAYBeFAlU5i38FAgMB\n"\
"AAGjgfAwge0wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMCAYYwHQYDVR0O\n"\
"BBYEFJxfAN+qAdcwKziIorhtSpzyEZGDMB8GA1UdIwQYMBaAFL9ft9HO3R+G9FtV\n"\
"rNzXEMIOqYjnME8GCCsGAQUFBwEBBEMwQTAcBggrBgEFBQcwAYYQaHR0cDovL28u\n"\
"c3MyLnVzLzAhBggrBgEFBQcwAoYVaHR0cDovL3guc3MyLnVzL3guY2VyMCYGA1Ud\n"\
"HwQfMB0wG6AZoBeGFWh0dHA6Ly9zLnNzMi51cy9yLmNybDARBgNVHSAECjAIMAYG\n"\
"BFUdIAAwDQYJKoZIhvcNAQELBQADggEBACMd44pXyn3pF3lM8R5V/cxTbj5HD9/G\n"\
"VfKyBDbtgB9TxF00KGu+x1X8Z+rLP3+QsjPNG1gQggL4+C/1E2DUBc7xgQjB3ad1\n"\
"l08YuW3e95ORCLp+QCztweq7dp4zBncdDQh/U90bZKuCJ/Fp1U1ervShw3WnWEQt\n"\
"8jxwmKy6abaVd38PMV4s/KCHOkdp8Hlf9BRUpJVeEXgSYCfOn8J3/yNTd126/+pZ\n"\
"59vPr5KW7ySaNRB6nJHGDn2Z9j8Z3/VyVOEVqQdZe4O/Ui5GjLIAZHYcSNPYeehu\n"\
"VsyuLAOQ1xk4meTKCRlb/weWsKh/NEnfVqn3sF/tM+2MR7cwA130A4w=\n"\
"-----END CERTIFICATE-----"
#endif

#if 0
// Starfield Services Root Certificate Authority - G2
#define STARFIELD_ROOT_CA_G2 \
"-----BEGIN CERTIFICATE-----\n"\
"MIIFEjCCBHugAwIBAgICAQwwDQYJKoZIhvcNAQEFBQAwgbsxJDAiBgNVBAcTG1Zh\n"\
"bGlDZXJ0IFZhbGlkYXRpb24gTmV0d29yazEXMBUGA1UEChMOVmFsaUNlcnQsIElu\n"\
"Yy4xNTAzBgNVBAsTLFZhbGlDZXJ0IENsYXNzIDIgUG9saWN5IFZhbGlkYXRpb24g\n"\
"QXV0aG9yaXR5MSEwHwYDVQQDExhodHRwOi8vd3d3LnZhbGljZXJ0LmNvbS8xIDAe\n"\
"BgkqhkiG9w0BCQEWEWluZm9AdmFsaWNlcnQuY29tMB4XDTA0MDYyOTE3MzkxNloX\n"\
"DTI0MDYyOTE3MzkxNlowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0YXJmaWVs\n"\
"ZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBDbGFzcyAy\n"\
"IENlcnRpZmljYXRpb24gQXV0aG9yaXR5MIIBIDANBgkqhkiG9w0BAQEFAAOCAQ0A\n"\
"MIIBCAKCAQEAtzLI/ulxpgSFrQwRZN/OTe/IAxiHP6Gr+zymn/DDodrU2G4rU5D7\n"\
"JKQ+hPCe6F/s5SdE9SimP3ve4CrwyK9TL57KBQGTHo9mHDmnTfpatnMEJWbrd3/n\n"\
"WcZKmSUUVOsmx/N/GdUwcI+vsEYq/63rKe3Xn6oEh6PU+YmlNF/bQ5GCNtlmPLG4\n"\
"uYL9nDo+EMg77wZlZnqbGRg9/3FRPDAuX749d3OyXQZswyNWmiuFJpIcpwKz5D8N\n"\
"rwh5grg2Peqc0zWzvGnK9cyd6P1kjReAM25eSl2ZyR6HtJ0awNVuEzUjXt+bXz3v\n"\
"1vd2wuo+u3gNHEJnawTY+Nbab4vyRKABqwIBA6OCAfMwggHvMB0GA1UdDgQWBBS/\n"\
"X7fRzt0fhvRbVazc1xDCDqmI5zCB0gYDVR0jBIHKMIHHoYHBpIG+MIG7MSQwIgYD\n"\
"VQQHExtWYWxpQ2VydCBWYWxpZGF0aW9uIE5ldHdvcmsxFzAVBgNVBAoTDlZhbGlD\n"\
"ZXJ0LCBJbmMuMTUwMwYDVQQLEyxWYWxpQ2VydCBDbGFzcyAyIFBvbGljeSBWYWxp\n"\
"ZGF0aW9uIEF1dGhvcml0eTEhMB8GA1UEAxMYaHR0cDovL3d3dy52YWxpY2VydC5j\n"\
"b20vMSAwHgYJKoZIhvcNAQkBFhFpbmZvQHZhbGljZXJ0LmNvbYIBATAPBgNVHRMB\n"\
"Af8EBTADAQH/MDkGCCsGAQUFBwEBBC0wKzApBggrBgEFBQcwAYYdaHR0cDovL29j\n"\
"c3Auc3RhcmZpZWxkdGVjaC5jb20wSgYDVR0fBEMwQTA/oD2gO4Y5aHR0cDovL2Nl\n"\
"cnRpZmljYXRlcy5zdGFyZmllbGR0ZWNoLmNvbS9yZXBvc2l0b3J5L3Jvb3QuY3Js\n"\
"MFEGA1UdIARKMEgwRgYEVR0gADA+MDwGCCsGAQUFBwIBFjBodHRwOi8vY2VydGlm\n"\
"aWNhdGVzLnN0YXJmaWVsZHRlY2guY29tL3JlcG9zaXRvcnkwDgYDVR0PAQH/BAQD\n"\
"AgEGMA0GCSqGSIb3DQEBBQUAA4GBAKVi8afCXSWlcD284ipxs33kDTcdVWptobCr\n"\
"mADkhWBKIMuh8D1195TaQ39oXCUIuNJ9MxB73HZn8bjhU3zhxoNbKXuNSm8uf0So\n"\
"GkVrMgfHeMpkksK0hAzc3S1fTbvdiuo43NlmouxBulVtWmQ9twPMHOKRUJ7jCUSV\n"\
"FxdzPcwl\n"\
"-----END CERTIFICATE-----"
#endif

#define S3_RANGE_RESPONSE_PREFIX "bytes 0-0/"
// 9 megabytes will be 7 digits
#define DATA_BYTE_SIZE_CHAR_MAX (sizeof(S3_RANGE_RESPONSE_PREFIX) + 7)

// NOTE: If this chunk size is 4k or more, this error happens during initial chunk download:
// Failed to read data: Error: SSL - Bad input parameters to function : <No-Low-Level-Code>. (mbedtls_transport.c:1649)
#define DATA_CHUNK_SIZE (1024 * 4)
/*
static buff_data_chunk[DATA_CHUNK_SIZE];
*/

#define HEADER_BUFFER_LENGTH 2048
static uint8_t buff_headers[HEADER_BUFFER_LENGTH];

#define RESPONSE_BUFFER_LENGTH (DATA_CHUNK_SIZE + 2048) /* base response buffer on chunk size and add a little extra */
static uint8_t buff_response[RESPONSE_BUFFER_LENGTH];



static void setup_request(HTTPRequestInfo_t* request, const char* method, const char* host, const char* path) {
    request->pMethod = method;
    request->methodLen = strlen(method);
    request->pPath = path;
    request->pathLen = strlen(path);
    request->pHost = host;
    request->hostLen = strlen(host);
    request->reqFlags = HTTP_REQUEST_KEEP_ALIVE_FLAG;
}

static void https_download_fw(const char* host, const char* path) {
	TlsTransportStatus_t tls_transport_status;
	HTTPStatus_t http_status;
	OtaPalStatus_t pal_status;
	const char * alpn_protocols[] = {  NULL };

    NetworkContext_t* network_conext = mbedtls_transport_allocate();
    if (NULL == network_conext) {
        LogError("Failed to allocate network context!");
        return;
    }

    //PkiObject_t ca_certificates[] = { PKI_OBJ_PEM((const unsigned char *)GODADDY_ROOT_CERTIFICATE_AUTHORITY_G2, sizeof(GODADDY_ROOT_CERTIFICATE_AUTHORITY_G2)) };
    //PkiObject_t ca_certificates[] = {PKI_OBJ_PEM((const unsigned char *)IOTCONNECT_BALTIMORE_CYBER_TRUST_ROOT, sizeof(IOTCONNECT_BALTIMORE_CYBER_TRUST_ROOT))};

    // use the same TLS cert as the MQTT connection cert for S3
    //PkiObject_t ca_certificates[] = {xPkiObjectFromLabel( TLS_ROOT_CA_CERT_LABEL)};

	PkiObject_t ca_certificates[] = {PKI_OBJ_PEM((const unsigned char *)STARFIELD_ROOT_CA_G2, sizeof(STARFIELD_ROOT_CA_G2))};

    /* ALPN protocols must be a NULL-terminated list of strings. */
    tls_transport_status = mbedtls_transport_configure(
        network_conext,
		alpn_protocols,
        NULL,
        NULL,
        ca_certificates,
        1
    );
    if( TLS_TRANSPORT_SUCCESS != tls_transport_status) {
        LogError("Failed to configure mbedtls transport! Error: %d", tls_transport_status);
        return;
    }

    tls_transport_status = mbedtls_transport_connect(
    	network_conext,
    	host,
        443,
        10000,
		10000
    );
    if (TLS_TRANSPORT_SUCCESS != tls_transport_status) {
        LogError("HTTPS: Failed to connect! Error: %d", tls_transport_status);
        return;
    }

    TransportInterface_t transport_if = {0};
	transport_if.pNetworkContext = network_conext;
	transport_if.send = mbedtls_transport_send;
	transport_if.recv = mbedtls_transport_recv;

    static HTTPResponse_t response = {0};
    response.pBuffer = buff_response;
    response.bufferLen = sizeof(buff_response);

    HTTPRequestHeaders_t headers = {0};
    headers.pBuffer = buff_headers;
	headers.bufferLen = sizeof(buff_headers);


    // When using S3, use a GET with range 0-0 and then the returned size will be like bytes 0-0/XXXX where X is the actual size
	// When using Azure Blob, use a HEAD with the URL in question and the Content-Length will contain the size.
    HTTPRequestInfo_t request = { 0 };
    setup_request(&request, HTTP_METHOD_GET, host, path);

    http_status = HTTPClient_InitializeRequestHeaders( &headers, &request);
	if (0 != http_status) {
    	LogError("HTTP failed to initialize headers! Error: %s", HTTPClient_strerror(http_status));
    	return;
	}

	http_status = HTTPClient_AddRangeHeader(&headers, 0, 0);
	if (0 != http_status) {
		LogError("HTTP failed to add initial range header for size query! Error: %s", HTTPClient_strerror(http_status));
		return;
	}

	http_status = HTTPClient_Send(
		&transport_if,
		&headers, /* HTTPRequestHeaders_t  pRequestHeaders*/
		NULL, /*const uint8_t * pRequestBodyBuf*/
		0, /* size_t reqBodyBufLen*/
		&response,
		0 /* uint32_t sendFlags*/
	);
	if (0 != http_status) {
    	LogError("HTTP Send Error: %s", HTTPClient_strerror(http_status));
	}

	// NOTE: AWS S3 may be returning Content-Range
	const char* data_length_str = NULL;
	size_t data_length_str_len = 0;

	// When using S3, use a GET with range 0-0 and then the returned size will be like bytes 0-0/XXXX where X is the actual size
	http_status = HTTPClient_ReadHeader( &response,
		"Content-Range",
		sizeof("Content-Range") - 1,
		&data_length_str,
		&data_length_str_len
	);
	if (0 != http_status) {
    	LogError("HTTP Error while obtaining headers: %s", HTTPClient_strerror(http_status));
	}

	if (response.statusCode != 200) {
		LogInfo("Response status code is: %u", response.statusCode);
	}

	if (NULL == data_length_str || 0 == data_length_str_len) {
		LogInfo("Could not obtain data length!");
		return;
	}

	LogInfo("Response range reported: %.*s", data_length_str_len, data_length_str);

	if (data_length_str_len > DATA_BYTE_SIZE_CHAR_MAX) {
		LogInfo("Unsupported data length: %lu", data_length_str_len);
		return;
	}

	//LogInfo("Response body: %.*s", response.bodyLen, response.pBody);

	int data_length = 0;
	char data_length_buffer[DATA_BYTE_SIZE_CHAR_MAX + 1]; // for scanf to deal with a null terminated string
	strncpy(data_length_buffer, data_length_str, data_length_str_len);
	if (1 != sscanf(data_length_buffer, S3_RANGE_RESPONSE_PREFIX"%d", &data_length)) {
		LogInfo("Could not convert data length to number");
		return;
	}

	LogInfo("Response data length (number) is %d", data_length);

	OtaFileContext_t file_context;
	file_context.fileSize = (uint32_t)data_length;
	file_context.pFilePath = (uint8_t *)"b_u585i_iot02a_ntz.bin";
	file_context.filePathMaxSize = (uint16_t)strlen((const char*)file_context.pFilePath);

	pal_status = otaPal_CreateFileForRx(&file_context);
	if (OtaPalSuccess != pal_status) {
		LogError("OTA failed to create file. Error: 0x%x", pal_status);
	}
	// OtaPalImageState_t image_state = otaPal_GetPlatformImageState( OtaFileContext_t * const pFileContext );

	int progress_ctr = 0;
	for (int data_start = 0; data_start < data_length; data_start += DATA_CHUNK_SIZE) {
		int data_end = data_start + DATA_CHUNK_SIZE;
		if (data_end > data_length) {
			data_end = data_length;
		}

		memset(&request, 0, sizeof(request));
		memset(&headers, 0, sizeof(headers));
	    headers.pBuffer = buff_headers;
		headers.bufferLen = sizeof(buff_headers);

	    setup_request(&request, HTTP_METHOD_GET, host, path);

	    http_status = HTTPClient_InitializeRequestHeaders(&headers, &request);
		if (HTTPSuccess != http_status) {
	    	LogError("HTTP failed to initialize headers! Error: %s", HTTPClient_strerror(http_status));
	    	return;
		}
		http_status = HTTPClient_AddRangeHeader(&headers, data_start, data_end - 1);
		if (HTTPSuccess != http_status) {
			LogError("HTTP failed to add range header! Error: %s", HTTPClient_strerror(http_status));
			return;
		}

		int tries_remaining = 30;
        do {
            http_status = HTTPClient_Send(
                &transport_if,
                &headers, /* HTTPRequestHeaders_t  pRequestHeaders*/
                NULL, /*const uint8_t * pRequestBodyBuf*/
                0, /* size_t reqBodyBufLen*/
                &response,
                0 /* uint32_t sendFlags*/
            );

            // we need to get at least one successful fetch, and if we do we can try back off.
            // this part will trigger on 100th try.
            if (0 != data_start && HTTPNetworkError == http_status) {
                if (0 == tries_remaining) {
                    LogError("HTTP range %d-%d send error: %s", data_start, data_end - 1, HTTPClient_strerror(http_status));
                    return;
                }
                LogError("Failed to get chunk range %d-%d. Reconnecting...", data_start, data_end - 1);
                vTaskDelay( 1000 );
                mbedtls_transport_disconnect(network_conext);
                tls_transport_status = mbedtls_transport_connect(
                	network_conext,
                	host,
                    443,
                    10000,
            		10000
                );
                tries_remaining--;
            } else if (HTTPSuccess != http_status) {
                LogError("HTTP range %d-%d send error: %s", data_start, data_end - 1, HTTPClient_strerror(http_status));
                return;
            }
        } while (http_status == HTTPNetworkError);

		if (progress_ctr % 30 == 29) {
		    LogInfo("Progress %d%%...", data_start * 100 / data_length);
			progress_ctr = 0;
		} else {
			progress_ctr++;
		}


	    int16_t bytes_written = otaPal_WriteBlock(
	    	&file_context,
			(uint32_t) data_start,
			(uint8_t*) response.pBody,
			(uint32_t) response.bodyLen
	    );
	    if (bytes_written != (int16_t) response.bodyLen) {
	    	LogError("Expected to write %d bytes, but wrote %u!", response.bodyLen, bytes_written);
	    	return;
	    }
	}
    mbedtls_transport_disconnect(network_conext);
    vTaskDelay(500);

    LogInfo("OTA download complete. Launching the new image!");

    pal_status = otaPal_CloseFile(&file_context);
	if (OtaPalSuccess != pal_status) {
		LogError("OTA failed close the downloaded firmware file. Error: 0x%x", pal_status);
	}

    vTaskDelay(100);

	pal_status = otaPal_ActivateNewImage(&file_context);
	if (OtaPalSuccess != pal_status) {
		LogError("OTA failed activate the downloaded firmware. Error: 0x%x", pal_status);
	}
    vTaskDelay(100);

}

#include "sys_evt.h"
#include "core_mqtt_agent.h"
#include "subscription_manager.h"
#include "mqtt_agent_task.h"

/*
AWS S3 does not have an official limit for the length of a presigned URL. However, some have encountered a presigned URL for an S3 object that was 1669 characters long, which is close to the unofficial URL length limit of 2 KB.
Presigned URLs (PUT & GET) do not support limiting the file size. A PUT HTTP request using the presigned URL is a "single"-part upload, and the object size is limited to 5 GB.
Signed URLs are generated with specific access permissions, expiration times, and cryptographic signatures. This ensures that only authorized users can access the content.
*/
#define JSON_OBJ_URL "\"url\":\""
#define JSON_OBJ_FILENAME "\"fileName\":\""
#define MAX_URL_LEN 2000
char url_buff[MAX_URL_LEN + 1];
static bool copy_until_char(char * target, const char* source, char terminator) {
    size_t src_len = strlen(source);
    target[0] = 0;
    for(size_t i = 0; i < src_len; i++) {
        int src_ch = source[i];
        if (terminator == src_ch) {
            target[i] = 0;
            return true;
        } else {
            target[i] = source[i];
        }
    }
    return false; // ran past the end of source
}
static void on_c2d_message( void * subscription_context, MQTTPublishInfo_t * publish_info ) {
    (void) subscription_context;

    if (!publish_info) {
        LogError("on_c2d_message: Publish info is NULL?");
        return;
    }
    LogInfo("<<< %.*s", publish_info->payloadLength, publish_info->pPayload);
    char* payload = (char *)publish_info->pPayload;
    payload[publish_info->payloadLength] = 0; // terminate the string just in case. Don't really care about the last char for now
    char *url_part = strstr(payload, JSON_OBJ_URL);
    if (!url_part) {
        LogInfo("on_c2d_message: command received");
        return;
    }
    LogInfo("on_c2d_message: OTA received");
    if (!copy_until_char(url_buff, &url_part[strlen(JSON_OBJ_URL)], '"')) {
        LogError("on_c2d_message: Publish info is NULL?");
    }
    LogInfo("URL: %s", url_buff);

    char file_name_buff[100]; // TODO: limit file name length
    char *fn_part = strstr(publish_info->pPayload, JSON_OBJ_FILENAME);
    if (!fn_part) {
        LogInfo("on_c2d_message: missing filename?");
        return;
    }
    if (!copy_until_char(file_name_buff, &fn_part[strlen(JSON_OBJ_FILENAME)], '"')) {
        LogError("on_c2d_message: Publish info is NULL?");
    }
    LogInfo("File: %s", file_name_buff);

}

#include "kvstore.h"
#define DEVICE_ID_MAX_LEN 129
#define TOPIC_STR_MAX_LEN (DEVICE_ID_MAX_LEN + 20)
static bool subscribe_to_c2d_topic(void)
{
    char device_id[DEVICE_ID_MAX_LEN];
    char sub_topic[TOPIC_STR_MAX_LEN];
    if (KVStore_getString(CS_CORE_THING_NAME, device_id, DEVICE_ID_MAX_LEN) <= 0) {
	    LogError("Unable to get device ID");
	    return false;
	}
    sprintf(sub_topic, "iot/%s/cmd", device_id);

    MQTTAgentHandle_t agent_handle = xGetMqttAgentHandle();
    if (agent_handle == NULL )  {
	    LogError("Unable to get agent handle");
	    return false;
    }

    MQTTStatus_t mqtt_status = MqttAgent_SubscribeSync( agent_handle,
		sub_topic,
		1 /* qos */,
		on_c2d_message,
		NULL
    );
    if (MQTTSuccess != mqtt_status) {
        LogError("Failed to SUBSCRIBE to topic with error = %u.", mqtt_status);
        return false;
    }

    LogInfo("Subscribed to topic %s.\n\n", sub_topic);

    return true;
}

static bool is_mqtt_connected(void)
{
	/* Wait for MQTT to be connected */
	EventBits_t uxEvents = xEventGroupWaitBits(xSystemEvents,
											   EVT_MASK_MQTT_CONNECTED,
											   pdFALSE,
											   pdTRUE,
											   0);

	return ((uxEvents & EVT_MASK_MQTT_CONNECTED) == EVT_MASK_MQTT_CONNECTED);
}

void vIOTC_Ota_Handler(void *parameters) {
    (void) parameters;

    vTaskDelay( 15000 );
#if 0
    while (!is_mqtt_connected()) {
        vTaskDelay( 1000 );
    }
    subscribe_to_c2d_topic();
#endif

#if 0
			/* Write to */
			bytesWritten = snprintf(payloadBuf, (size_t)MQTT_PUBLISH_MAX_LEN,
					"{\"d\":[{\"d\":{\"version\":\"MLDEMO-1.0\",\"class\":\"%s\"}}],\"mt\":0,\"cd\":\"XG4E2EW\"}",
					sAiClassLabels[max_idx]);
#endif
    // http_download_fw("saleshosted.z13.web.core.windows.net", "/demo/st/b_u585i_iot02a_ntz-orig.bin");
    https_download_fw("iotc-260030673750.s3.amazonaws.com", "/584af730-2854-4a77-8f3b-ca1696401e08/firmware/9db499d2-7f9a-47ac-a0ca-c46a94f58161/2a4dd76c-09d6-4c7a-99cc-24478443e758.iso?AWSAccessKeyId=ASIATZCYJGNLFRTG3N5Z&Expires=1699122284&x-amz-security-token=IQoJb3JpZ2luX2VjEBMaCXVzLWVhc3QtMSJIMEYCIQD5o%2B51dIMReUYskkSs4VCz%2F0TEyqgzNvGR1IVU%2F%2FPYwQIhAKUuTmeUfP3hmF3d4mCaektX3JZsn5FX1Z%2B7K0f%2BlnGyKugCCEwQABoMMjYwMDMwNjczNzUwIgzLg9HIaVgG77SpDJAqxQIX8kwAsgRKSaHKGv2Ha5yQYuz5ad8itdfbaCtObAkt3YvM5u9hSXClO5el3E8CX88zHVp%2BJTOkUnubh4Jg614ircETeTocxQb4yC%2Bx6Smb%2FCWzYynF6ImE0e6uMkZex6DdPUVTYCz7aIwCtdxvFFDHEIBixdnEBAFX%2BBaJQ8cNXlbDiwX%2FoZpixDPbNpOsxiOuGfHd%2BT3di%2FSdUJkpinRV3KSXBNiWcsgwnD7hWRYpKAukHh99DFw0JwzR6dUaGbGWEs9K0x%2BQHjSlkoVbRh1plaWieWL9oQA3FLUc5Q8GkVEGlCXPrP20bG8pdTQDNEmcr2fW4aw17ekCEPCicS%2BeO0sZNpxlQO1vIXchH1NV%2B7E2Xv8pgCYS4bCJKMxID9oQiK317m0qQBdA41gii%2BCQTmRemkcWcXWbRYJcKzdufNhaRZ07MM70lKoGOp0BqJLymPpLpecIC0ZktF1Wsff8ThaXX0xKVc2iislODQ9I2wEPS6LoEM72EVjvgAkG3ThR57GETt7efiOfVyjVofXIErw8st8TbyM4y1HBRNFrI7%2FkRT6ntLjGnlA%2FXG08hjvMiSPQpuKnsEfJAkrTLAh9SuE4Km8o7vN01FvJHlAmkO6T5CmG%2B9DVZX4su%2F%2BIZ1nNG15%2F3IeIgx%2FHJg%3D%3D&Signature=BsHL4UPS3o91MP4WF38OWFA1mSE%3D");
	// LogInfo("HTTPS Test Done.");

    while (true) {
    	vTaskDelay(10000);
    }
}

