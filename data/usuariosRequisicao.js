// requisição assincrona
	var link = "http://192.168.1.1/";
	const TIMEOUT_CONEXAO = 5000; 
	var saidaFormatada = [];
	
	function novoLog(quantidade, offset) {
		return new Promise((resolve, reject) => {//'resolve' quando o sistema retornou 200 e 'reject' quando houve falha de timeout
			let xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function() {
				if (this.readyState == 4 && this.status == 200)
					resolve(JSON.parse(this.responseText));
			}
			xhttp.open("GET", link + "logs/" + "?quantidade=" + quantidade + "&offset=" + offset);
			xhttp.timeout = TIMEOUT_CONEXAO;
			xhttp.ontimeout = () => {reject(new Error("Timeout"))};
			xhttp.send();
		});
	}

	function listaUsuarios() {
		return new Promise((resolve, reject) => { //'resolve' quando o sistema retornou 200 e 'reject' quando houve falha de timeout
			let xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function() {
				if (this.readyState == 4 && this.status == 200)
					resolve(JSON.parse(this.responseText));
			}
			xhttp.open("GET", link + "usuarios/" + "?matricula=");
			xhttp.timeout = TIMEOUT_CONEXAO;
			xhttp.ontimeout = () => {reject(new Error("Timeout"))};
			xhttp.send();
		});
	}

	function quantidadeLogs() {
		return new Promise((resolve, reject) => {//'resolve' quando o sistema retornou 200 e 'reject' quando houve falha de timeout
			let xhttp = new XMLHttpRequest();
			xhttp.onreadystatechange = function() {
				if (this.readyState == 4 && this.status == 200)
					resolve(parseInt(this.responseText));
			}
			xhttp.open("GET", link + "logs/quantidade/");
			xhttp.timeout = TIMEOUT_CONEXAO;
			xhttp.ontimeout = () => {reject(new Error("Timeout"))};
			xhttp.send();
		});
	}

	async function obterLogs()
	{
		var quantidadeTotal = await quantidadeLogs();
		document.getElementById("barraProgresso").max=quantidadeTotal;
		const LIMITE = 500; //Limite de dados por requisicao
		var nPacotes = Math.ceil(quantidadeTotal/LIMITE); //Arredonda para cima
		var offset = 0;
		var logSaida = [];
		for (let i=0; i<nPacotes; i++)
		{
			document.getElementById("barraProgresso").value=offset+LIMITE;
			if (i == nPacotes-1) //Ultimo pacote?
				logSaida = logSaida.concat(await novoLog(quantidadeTotal-i*LIMITE, offset));
			else
				logSaida = logSaida.concat(await novoLog(LIMITE, offset));
			offset += LIMITE;
		}
		return logSaida;
	}

	function buscaTodosLogs()
	{
		const MSG_BLOQUEIO = "Bloqueio";
		const MSG_DESBLOQUEIO = "Desbloqueio";
		
		var data;
		listaUsuarios().then((listaUsuarios) => {
			obterLogs().then((saida) => {
				saida.forEach((value, index) => {
					data = new Date(value.epoch*1000);
					value.epoch = data.toLocaleDateString() + " " + data.toLocaleTimeString();
					if (value.acao == 0)
						value.acao = MSG_BLOQUEIO;
					else
						value.acao = MSG_DESBLOQUEIO;
					for (let i=0; i<listaUsuarios.length; i++)
					{
						if (value.matricula === listaUsuarios[i].matricula)
						{
							value.nome = listaUsuarios[i].nome;
							break;
						}							
					}
					saidaFormatada = saidaFormatada.concat(value);
				});
				console.log(saidaFormatada);
			})
			.catch(err => {
				//Tratar o erro de timeout
				console.log("Erro de timeout: puxar o log");
				alert('Rede desconectada! Reconecte-se a rede Wi-Fi.');
				// criar elemento avisando do erro
			})
		})
		.catch(err => {
			//Tratar o erro de timeout
			console.log("Erro de timeout: pegar lista de usuarios");
			alert('Rede desconectada! Reconecte-se a rede Wi-Fi');
			// criar elemento avisando do erro
		})		
	}
