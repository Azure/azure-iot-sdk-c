PARAM ($EnvironmentVariableScope = "User", $IOTParametersFilePath = ".\iot_device_params.txt")

cat $IOTParametersFilePath | %{
	if (-NOT $_.TrimStart().StartsWith("#"))
	{
		$Tokens = $_.Split([char[]]@('='), 2);
		$Name = $Tokens[0];
		
		if (-NOT [String]::IsNullOrWhiteSpace($Name))
		{		
			if (-NOT [String]::IsNullOrWhiteSpace($Tokens[1]))
			{
				$Value = $Tokens[1].Replace("$", "`$");
				
				if ($Value.StartsWith("`"") -OR $Value.StartsWith("`'"))
				{
					$Value = $Value.Remove(0, 1);
				}

				if ($Value.EndsWith("`"") -OR $Value.EndsWith("`'"))
				{
					$Value = $Value.Remove($Value.Length - 1, 1);
				}
			}
		
			Write-Host "Setting $Name=$Value";
			[Environment]::SetEnvironmentVariable($Name, $Value, $EnvironmentVariableScope);
		}
	}
}

Write-Host "Completed setting Environment Variables";