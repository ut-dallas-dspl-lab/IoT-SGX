# enable SGX memory measurement tool
enable sgx_emmt

# exit GDB after successfull execution
set $_exitcode = -999
define hook-stop
  if $_exitcode != -999
    quit
  end 
end
