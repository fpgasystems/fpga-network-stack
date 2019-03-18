
$IP_CORES = "ip_handler", "mac_ip_encode", "arp_server_subnet", "icmp_server", "toe", "echo_server_application", "ethernet_frame_padding", "iperf_client", "udp", "ipv4", "iperf_udp_client", "dhcp_client"

$HLS_DIR = $PSScriptRoot

if ($args.Count -eq 1) { 
    if ($args.Get(0) -eq "vc709") {
        $PART = "xc7vx690tffg1761-2"
        Write-Output "Compiling for $PART"
    } 
    
    elseif ($args.Get(0) -eq "vcu118") {
        $PART = "xcvu9p-flga2104-2L-e"
        Write-Output "Compiling for $PART"
    } 
    
    else {
        Write-Output "Part not supported!"
        exit
    }

} 

else {
    Write-Output "Argument missing!"
    exit
}

$IP_REPO = "$HLS_DIR\..\iprepo"


if (!(Test-Path -Path $IP_REPO)){

    mkdir $IP_REPO
    Write-Output "IP repo created at $IP_REPO!"
}


foreach ($IP in $IP_CORES) {

    Write-Output "---------------------------------------------"
    Write-Output "Compiling for $IP"
    Write-Output "---------------------------------------------"

	
    $TCL_FILE = "$HLS_DIR\" + $IP + "\run_hls.tcl"

    if ([System.IO.File]::Exists($TCL_FILE)) {
        $NEW_LINE = "set_part {" + $PART + "}"
        $regex = 'set_part {.*}'
        (Get-Content $TCL_FILE) -replace $regex, $NEW_LINE | Set-Content $TCL_FILE
        
        Set-Location "$HLS_DIR\$IP"

        &vivado_hls -f run_hls.tcl

        if (!(Test-Path -Path "$IP_REPO\$IP")){
            mkdir "$IP_REPO\$IP"
        }
        else {
            Remove-Item -LiteralPath "$IP_REPO\$IP" -Force -Recurse
            mkdir "$IP_REPO\$IP"
        }

        $ZIP_PATH = "$HLS_DIR\$IP\$IP" + "_prj\solution1\impl\ip"

        $FILES = Get-ChildItem $ZIP_PATH -Filter *.zip 

         
        if ($FILES.Count -eq 1){
             Write-Output $files[0].FullName
             $OUTDIR = "$IP_REPO\$IP\" + [io.path]::GetFileNameWithoutExtension($files[0].FullName)
             mkdir $OUTDIR

             Expand-Archive $files[0].FullName -DestinationPath $OUTDIR
        }
        else {
            Write-Output "The output .zip file for $IP could not be found!"
            Write-Output "Did the build fail?"

        }

        

    }
    else {
        Write-Output "No .tlc file were found for the IP!"
        Write-Output "The IP will not be generated"
        Write-Output "The expected path was $TCL_FILE"
    }

}

Write-Output "Generated and copied all HLS IPs to ip repository."
Write-Output "Go to the projects directory and run vivado -mode batch -source create_<board>_proj.tcl to create the vivado project"



