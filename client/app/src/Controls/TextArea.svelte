<script>
    import { onMount } from "svelte"

    let { value = $bindable(""), error = null, class: className = "", ...restProps } = $props()

    let textAreaElement
    let isFocused = $state(false)

    onMount(() => {
        if (textAreaElement.hasAttribute("autofocus")) {
            textAreaElement.focus()
        }
    })

    let classes = $derived([error ? "pe-7 dark:border-red-400 border-red-600 focus:border-red-500 focus:ring-red-600 hover:border-red-400 dark:hover:border-red-600" : "border-gray-300 dark:border-gray-700 hover:border-aya-300 focus:border-aya-500 dark:border-zinc-700 dark:focus:border-aya-600", className, "focus:ring-aya-500 block w-full rounded-lg border-gray-300 transition duration-100 dark:bg-zinc-900 dark:text-gray-300"].filter(Boolean).join(" "))
    
    let iconColor = $derived(isFocused ? "text-red-500 dark:text-red-600" : "text-red-600 dark:text-red-400")
</script>

<div class="relative {className && className.includes('w-full') ? 'w-full' : ''}">
    <textarea class={classes} bind:value bind:this={textAreaElement} onfocus={() => (isFocused = true)} onblur={() => (isFocused = false)} {...restProps} />
    {#if error}
        <i class={`fas fa-times pointer-events-none absolute right-3 top-1/2 -translate-y-1/2 transform transition duration-100 ${iconColor}`} />
    {/if}
</div>