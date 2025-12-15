<script>
    let { href = null, text = null, class: className = "", ...restProps } = $props();
</script>

<button href={href} class="aya-anim-pop rounded-lg text-aya-500 underline transition duration-200 hover:text-aya-400 {className}" {...restProps}>
    {text}
</button>
