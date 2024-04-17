/*
 * Authentification
 * ----------------
 *
 * On récupère la clé d’API et l’utilisateur courant depuis le fragment de
 * l’URL. Il doit être de la forme `user:api_key`. L’utilisateur ne sert pas à
 * l’authentification mais permet de savoir qui paie. Le vrai utilisateur est
 * noté dans le journal côté API.
 *
 */

const [me, api_key] = document.location.hash.substring(1).split(":");

let you;
switch (me) {
	case 'riku': you = 'anju'; break;
	case 'anju': you = 'riku'; break;
	default: document.location = 'welcome.html';
}

/*
 * File de reçu
 * ------------
 *
 * Quand une photo est envoyée, on ajoute tous les reçus lus dans la file de
 * reçus en attente. Le bouton Effacer se transforme alors en bouton Abandonner
 * et on affiche le nombre de reçus restants.
 *
 * À chaque envoi ou abandon de reçu, on remplit le formulaire avec les données
 * du reçu suivant.
 *
 */

const receiptQueue = [];

function shiftReceipt() {
	const receipt = receiptQueue.shift();
	if (!receipt) {
		// Revient au mode manuel.
		clearEntryButton.style.display = "inline-flex";
		discardEntryButton.style.display = "none";
		queueCounter.style.display = "none";
		return false;
	}

	entry.reset();
	for (const key in receipt)
		entry[key].value = receipt[key] || null;
	
	// Passe au mode semi-automatique avec affichage de l’état de la file.
	clearEntryButton.style.display = "none";
	discardEntryButton.style.display = "inline-flex";
	queueCounter.innerText = `あと ${receiptQueue.length} 枚`;
	queueCounter.style.display = "inline";

	return true;
}

// Si on reçoit plusieurs reçus avec le même numéro d’inscription, et que le
// premier est sauvegardé avec une catégorie et une remarque, on associe cette
// catégorie et remarque à tous les autres reçus en attente.
function updateReceiptsStore(registration, category, remark) {
	for (const receipt of receiptQueue) {
		if (receipt.registration == registration) {
			receipt.category = category;
			receipt.remark = remark;
		}
	}
}

/*
 * Initialisation du formulaire
 * ----------------------------
 *
 * À supposer qu’on se mette à utiliser l’application pour enregistrer les
 * reçus le jour J, il sera pratique d’avoir par défaut la date courante. On la
 * remplit alors automatiquement au chargement de la page, ou après une longue
 * inactivité.
 *
 * Quand l’application passe en premier-plan, on focus le prix car c’est là que
 * tout démarre en pratique. Si en plus il s’est passé une heure depuis le
 * dernier focus et qu’il n’y a pas de données remplies, on reset le form pour
 * avoir la date courante et la catégorie par défaut.
 *
 */

function today() {
	return new Date().toISOString().split("T")[0];
}

entry.date.value = today();

let lastFocus = Date.now();

window.onfocus = () => {
	const now = Date.now();
	if (now - lastFocus > 3600000 && !entry.amount.value && receiptQueue.length == 0) {
		entry.reset();
		entry.date.value = today();
	}
	lastFocus = now;

	if (!entry.amount.value)
		entry.amount.focus();
};

// Quand le dialogue de sélection de facture est validé, on remplit le champ
// remarque avec le type de facture choisi.
bill.onsubmit = (event) => {
	entry.remark.value = event.submitter.value;
	billDialog.close();
	return false;
};

/**
 * Envoi de photo
 * --------------
 *
 * En haut à droite de la page, on affiche un bouton d’envoi de fichier et
 * éventuellement une icône d’appareil photo. Ces deux boutons sont associés au
 * même <input type="file"> qu’il reconfigurent au besoin.
 *
 * Dès qu’un fichier est sélectionné, le formulaire est automatiquement envoyé,
 * ce qui génère une requête à l’API. On reçoit ensuite une liste de reçus qui
 * alimente la file de reçus.
 *
 */

selectPictureButton.onclick = () => {
	pictureSelector.removeAttribute("capture");
	pictureSelector.showPicker();
};

takePictureButton.onclick = () => {
	pictureSelector.setAttribute("capture", "environment");
	pictureSelector.showPicker();
};

// pictureSelector.capture n’est pas défini sur les navigateurs qui ne
// supportent pas la prise de photo. La présence donc de la propriété nous
// permet donc de savoir quand afficher l’icone d’appareil photo.
if (pictureSelector.capture)
	takePictureButton.style.display = "inline-flex";

uploadForm.onsubmit = (event) => {
	event.preventDefault();
	selectPictureButton.disabled = true;
	takePictureButton.disabled = true;

	fetch(`api/upload?key=${api_key}`, {
		method: "POST",
		body: new FormData(uploadForm),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		const hadReceipts = receiptQueue.length > 0;
		for (const receipt of json.receipts)
			receiptQueue.push(receipt);
		if (!hadReceipts)
			shiftReceipt();
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		selectPictureButton.disabled = false;
		takePictureButton.disabled = false;
	})
};

/*
 * Avion
 * -----
 *
 * L’avion est une petite icône qui se déplace du bouton d’envoi vers le bouton
 * d’historique, pour offrir un retour visuel quand le reçu a bien été envoyé.
 *
 */

let plane = null;

function flyPlane() {
	plane?.remove();
	plane = document.createElement("span");
	plane.className = "plane";
	centerTo(plane, submitEntryButton);
	document.body.appendChild(plane);
	centerTo(plane, openHistoryButton);
}

function centerTo(element, reference) {
	with (reference) {
		element.style.position = "absolute";
		element.style.top = offsetTop + offsetHeight / 2 + "px";
		element.style.left = offsetLeft + offsetWidth / 2 + "px";
	}
}

/*
 * Historique
 * ----------
 *
 * À chaque envoi, on ajoute une ligne à l’historique. L’historique sert à
 * éviter les doublons, mais aussi à effacer les données erronées quand on s’en
 * rend compte juste après l’envoi.
 *
 * L’historique se limite aux reçus de la session courante. Pour aller plus
 * loin dans le passé, il faut aller directement sur le serveur et éditer le
 * journal d’opérations.
 *
 * La modale d’historique propose un bouton de téléchargement qui contient
 * l’intégralité des opérations et pas juste celles affichées.
 *
 */

const amountFormatter = Intl.NumberFormat("ja-JP", { signDisplay: "exceptZero" });

class HistoryEntry {
	#row;
	#withdrawButton;

	constructor(data) {
		this.data = data;

		const dateCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		dateCell.innerText = data.date.replace(/^\d+-0?(\d+)-0?(\d+)$/, "$1月$2日");

		const amountCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		amountCell.innerText = amountFormatter.format(data[me]) + "円";
		amountCell.title = data.category;

		const actionsCell = document.createElement("td");
		dateCell.style.textAlign = "right";
		this.#withdrawButton = document.createElement("button");
		this.#withdrawButton.innerText = "取消";
		this.#withdrawButton.onclick = () => { this.withdraw(); };
		actionsCell.appendChild(this.#withdrawButton);

		this.#row = document.createElement("tr");
		this.#row.appendChild(dateCell);
		this.#row.appendChild(amountCell);
		this.#row.appendChild(actionsCell);
		historyTable.appendChild(this.#row);
	}

	withdraw() {
		this.#withdrawButton.disabled = true;

		fetch(`api/withdraw?key=${api_key}`, {
			method: "POST",
			headers: { "Content-Type": "application/json" },
			body: JSON.stringify({ id: this.data.id }),
		}).then((response) => {
			if (response.ok)
				return response.json();
			else
				throw new Error("HTTP " + response.status);
		}).then((json) => {
			this.#row.classList.add("withdrawn");
			this.#withdrawButton.style.visibility = "hidden";
		}).catch((error) => {
			alert(error.message);
			this.#withdrawButton.disabled = false;
		})
	}
}

/*
 * Envoi
 * -----
 *
 * Bâtit le JSON à envoyer à l’API avec les données du formulaire. En cas de
 * réussite, on ajoute une entrée à l’historique et lance l’avion. En cas
 * d’erreur on affiche une alerte et laisse le formulaire intact.
 *
 * La traduction de l’unique champ *amount* en deux colonnes riku/anju se fait
 * côté front afin de pouvoir se donner la liberté plus tard de rendre
 * personnalisable les deux valeurs individuellement. Le front a aussi une
 * meilleure notion des catégories tandis que pour le back ce sont des chaines
 * obscures.
 *
 */

function buildEntryData() {
	const data = Object.fromEntries(new FormData(entry));
	const amount = Number(data.amount);
	delete data.amount;
	const selectedCategory = entry.querySelector("label:has(input[name=category]:checked)");
	switch (selectedCategory.className) {
		case 'expense':
			data[me] = -amount;
			data[you] = null;
			break;
		case 'income':
			data[me] = amount;
			data[you] = null;
			break;
		case 'transfer':
			data[me] = -amount;
			data[you] = amount;
			break;
		default:
			throw new Error("不正カテゴリー");
	}
	return data;
}

entry.onsubmit = () => {
	event.preventDefault();
	const data = buildEntryData();
	submitEntryButton.disabled = true;

	fetch(`api/send?key=${api_key}`, {
		method: "POST",
		headers: { "Content-Type": "application/json" },
		body: JSON.stringify(data),
	}).then((response) => {
		if (response.ok)
			return response.json();
		else
			throw new Error("HTTP " + response.status);
	}).then((json) => {
		data.id = json['id'];
		new HistoryEntry(data);
		flyPlane();

		// Partage le magasin avec les autres reçus de la file.
		if (entry.registration.value)
			updateReceiptsStore(entry.registration.value,
			                    entry.category.value,
			                    entry.remark.value);

		if (!shiftReceipt()) {
			// Réinitialise seulement partiellement le formulaire pour
			// faciliter l’ajout de reçus similaires.
			entry.amount.value = null;
			entry.remark.value = null;
			entry.registration.value = null;
			entry.amount.focus();
		}
	}).catch((error) => {
		alert(error.message);
	}).finally(() => {
		submitEntryButton.disabled = false;
	})
};
