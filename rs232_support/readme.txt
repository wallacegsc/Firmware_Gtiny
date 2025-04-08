Guideline:

    Terminal: python -m venv gEnv 

    Entrando no gEnv
        Terminal:
            Set-ExecutionPolicy -ExecutionPolicy Unrestricted -Scope Process  
            gEnv/Scripts/activate

        Com um script python aberto
            Selecionar interpretador do gEnv

        Verificar se está no gEnv
            pip list -> Terá o pip e libs padrão

    Instalando as libs
        pip install -r  requirements.txt
        pyserial, pycryptodome, tqdm, pytz, pandas

        Verificar se tudo foi instalado corretamente
            pip list


Se a COM e o conversor estiverem funcionando e o script não consegue comunicar:
	Tentar: Desinstalar e instalar CH340 Driver